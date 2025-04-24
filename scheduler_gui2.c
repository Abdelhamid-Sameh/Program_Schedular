#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "OS_MS2.h"
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

int pipe_fd[2]; // 0 = read, 1 = write
GThread *console_thread = NULL;

typedef enum
{
    STATE_INIT,
    STATE_RUNNING,
    STATE_PAUSED,
    STATE_FINISHED
} SimState;

static GtkWidget *btn_start, *btn_stop, *btn_step, *btn_finish, *btn_reset;

typedef struct
{
    GtkWidget *dropdown;
    GtkWidget *pid_label;
    GtkWidget *state_label;
    GtkWidget *priority_label;
    GtkWidget *mem_start_label;
    GtkWidget *mem_end_label;
    GtkWidget *pc_label;
} ProcessInfoWidgets;

typedef struct
{
    GtkWidget *clock_label;
    GtkWidget *ready_queues[4];
    GtkWidget *blocked_queue;
    GtkWidget *memory_table;
    GtkWidget *running_pid_label;
    GtkWidget *cur_inst_label;
    GtkWidget *proc_count_label;
    GtkWidget *time_in_queue_label;
    GtkWidget *mutex_tables[3];
    GtkWidget *mutex_states[3];
    GtkTextBuffer *console_buffer;
    ProcessInfoWidgets proc_info;
    GtkWidget *input_entry;
    GtkWidget *input_submit;
} UIElements;

static GtkWidget *file_entry;
static GtkWidget *quantum_spin;
static GtkWidget *quantum_label;
static GtkWidget *instructions_textview;
static GtkWidget *algo_combo;
GtkWidget *algo_label;
static GtkWidget *status_label;
static GtkWidget *arrival_spin;
static GtkWidget *main_window;
static GtkWidget *starter_window;
static UIElements ui;
static GThread *sim_thread = NULL;
static guint update_timer = 0;
static int lastProcessCount = -1;

// Forward declarations
static void create_simulation_window(int algorithm, int quantum);
static void update_gui();

// ===================== GUI UPDATING =====================
static gboolean update_all(gpointer data)
{
    update_gui();
    return TRUE;
}

static gboolean append_to_console(gpointer data)
{
    const char *text = (const char *)data;
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(ui.console_buffer, &iter);
    gtk_text_buffer_insert(ui.console_buffer, &iter, text, -1);
    g_free(data);
    return FALSE;
}

static gpointer console_output_thread(gpointer data)
{
    char buffer[256];
    while (1)
    {
        ssize_t bytes = read(pipe_fd[0], buffer, sizeof(buffer) - 1);
        if (bytes <= 0)
            break;
        buffer[bytes] = '\0';

        char *copy = g_strdup(buffer);
        g_idle_add(append_to_console, copy);
    }
    return NULL;
}

static void start_gui_updater()
{
    if (update_timer != 0)
        g_source_remove(update_timer);
    update_timer = g_timeout_add(500, update_all, NULL); // update every 500ms
}

static void stop_gui_updater()
{
    if (update_timer != 0)
    {
        g_source_remove(update_timer);
        update_timer = 0;
    }
}

static void set_buttons_state(SimState state)
{
    gtk_widget_set_sensitive(btn_start, state == STATE_INIT || state == STATE_PAUSED);
    gtk_widget_set_sensitive(btn_stop, state == STATE_RUNNING);
    gtk_widget_set_sensitive(btn_step, state != STATE_INIT && state != STATE_FINISHED);
    gtk_widget_set_sensitive(btn_finish, state != STATE_INIT && state != STATE_FINISHED);
    gtk_widget_set_sensitive(btn_reset, TRUE);
}

static void update_process_dropdown(GtkWidget *dropdown)
{
    int count = getProcessCount();
    if (count == lastProcessCount)
        return; // nothing changed, skip rebuilding

    // Remember old selection
    int old_active = gtk_combo_box_get_active(GTK_COMBO_BOX(dropdown));

    // Rebuild
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(dropdown));
    for (int i = 1; i <= count; i++)
    {
        char pid_str[11];
        snprintf(pid_str, sizeof(pid_str), "%d", i);
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(dropdown), pid_str);
    }

    // Restore selection safely
    int new_active = old_active < count ? old_active : (count > 0 ? 0 : -1);
    if (new_active >= 0)
        gtk_combo_box_set_active(GTK_COMBO_BOX(dropdown), new_active);

    lastProcessCount = count;
}

static void update_console(const char *text)
{
    GtkTextIter iter;
    gtk_text_buffer_get_end_iter(ui.console_buffer, &iter);
    gtk_text_buffer_insert(ui.console_buffer, &iter, text, -1);
}

static gpointer run_simulation(gpointer data)
{
    int *params = (int *)data;
    chooseSchd(params[0], params[1]);
    g_free(params);
    return NULL;
}

static void on_start_clicked(GtkButton *btn, gpointer d)
{
    start();
    set_buttons_state(STATE_RUNNING);
}
static void on_stop_clicked(GtkButton *btn, gpointer d)
{
    stop();
    set_buttons_state(STATE_PAUSED);
}
static void on_step_clicked(GtkButton *btn, gpointer d) { step(); }
static void on_finish_clicked(GtkButton *btn, gpointer d)
{
    finish();
    set_buttons_state(STATE_FINISHED);
}

static gboolean destroy_starter(gpointer data)
{
    gtk_widget_destroy(GTK_WIDGET(data));
    return FALSE;
}

static void on_reset_clicked(GtkButton *btn, gpointer d)
{
    stop_gui_updater(); // Stop periodic updates

    // 1. Signal simulation thread to stop
    simKillFlag = true;

    // 2. Wait for thread to exit (if it was running)
    if (sim_thread)
    {
        g_thread_unref(sim_thread);
        sim_thread = NULL;
    }

    // 3. Reset simulation state (optional)
    resetSimulation(); // We'll define this next

    // 4. Destroy simulation window
    if (main_window)
    {
        gtk_widget_destroy(main_window);
        main_window = NULL;
    }

    // 5. Show starter window again
    gtk_widget_show_all(starter_window);
}

static void on_proc_dropdown_changed(GtkComboBox *combo, gpointer user_data)
{
    update_gui();
}

static void on_input_submit_clicked(GtkButton *btn, gpointer data)
{
    const char *text = gtk_entry_get_text(GTK_ENTRY(ui.input_entry));
    if (strlen(text) > 0)
    {
        submitUserInput((char *)text);                     // from OS_MS2.c
        gtk_entry_set_text(GTK_ENTRY(ui.input_entry), ""); // clear
        gtk_widget_set_sensitive(ui.input_entry, FALSE);
        gtk_widget_set_sensitive(ui.input_submit, FALSE);
    }
}

static void create_simulation_window(int algorithm, int quantum)
{
    main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(main_window), "Scheduler Simulation");
    gtk_window_set_default_size(GTK_WINDOW(main_window), 1200, 700);
    g_signal_connect(main_window, "destroy", G_CALLBACK(on_reset_clicked), NULL);

    GtkWidget *main_grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(main_grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(main_grid), 10);
    gtk_container_add(GTK_CONTAINER(main_window), main_grid);

    // ========== LEFT SIDE ==========
    GtkWidget *left_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 30);
    gtk_grid_attach(GTK_GRID(main_grid), left_box, 0, 0, 1, 1);
    gtk_widget_set_hexpand(left_box, TRUE);

    // Top-left general info
    ui.proc_count_label = gtk_label_new("");
    gtk_box_pack_start(GTK_BOX(left_box), ui.proc_count_label, FALSE, FALSE, 0);

    ui.clock_label = gtk_label_new("Clock Cycle: 0");
    gtk_box_pack_start(GTK_BOX(left_box), ui.clock_label, FALSE, FALSE, 0);

    algo_label = gtk_label_new("Algorithm: [Unset]");
    gtk_box_pack_start(GTK_BOX(left_box), algo_label, FALSE, FALSE, 0);

    // Ready Queues
    for (int i = 0; i < 4; i++)
    {
        GtkWidget *frame = gtk_frame_new(i == 0 ? "Ready Queue 1" : i == 1 ? "Ready Queue 2"
                                                                : i == 2   ? "Ready Queue 3"
                                                                           : "Ready Queue 4");
        ui.ready_queues[i] = gtk_grid_new();
        gtk_container_add(GTK_CONTAINER(frame), ui.ready_queues[i]);
        gtk_box_pack_start(GTK_BOX(left_box), frame, FALSE, FALSE, 0);
    }

    // Blocked Queue
    GtkWidget *blocked_frame = gtk_frame_new("Blocked Queue");
    ui.blocked_queue = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(blocked_frame), ui.blocked_queue);
    gtk_box_pack_start(GTK_BOX(left_box), blocked_frame, FALSE, FALSE, 0);

    // Current Process Info
    GtkWidget *cur_proc_frame = gtk_frame_new("Current Process");
    GtkWidget *cur_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_add(GTK_CONTAINER(cur_proc_frame), cur_box);

    ui.running_pid_label = gtk_label_new("PID:");
    ui.cur_inst_label = gtk_label_new("Instruction:");
    ui.time_in_queue_label = gtk_label_new("Time In Queue:");

    gtk_box_pack_start(GTK_BOX(cur_box), ui.running_pid_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cur_box), ui.cur_inst_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(cur_box), ui.time_in_queue_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(left_box), cur_proc_frame, FALSE, FALSE, 0);

    // Mutex Queues + States
    char *mutex_names[3] = {"User Input", "User Output", "File"};
    for (int i = 0; i < 3; i++)
    {
        GtkWidget *frame = gtk_frame_new(mutex_names[i]);
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        ui.mutex_states[i] = gtk_label_new("State: Free");
        ui.mutex_tables[i] = gtk_grid_new();

        gtk_box_pack_start(GTK_BOX(vbox), ui.mutex_states[i], FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(vbox), ui.mutex_tables[i], FALSE, FALSE, 0);
        gtk_container_add(GTK_CONTAINER(frame), vbox);
        gtk_box_pack_start(GTK_BOX(left_box), frame, FALSE, FALSE, 0);
    }

    // --- User Input Section ---
    GtkWidget *input_frame = gtk_frame_new("Console Input");
    GtkWidget *input_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    ui.input_entry = gtk_entry_new();
    ui.input_submit = gtk_button_new_with_label("Submit");

    gtk_widget_set_sensitive(ui.input_entry, FALSE);
    gtk_widget_set_sensitive(ui.input_submit, FALSE);

    gtk_box_pack_start(GTK_BOX(input_box), ui.input_entry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(input_box), ui.input_submit, FALSE, FALSE, 0);
    gtk_container_add(GTK_CONTAINER(input_frame), input_box);
    gtk_box_pack_start(GTK_BOX(left_box), input_frame, FALSE, FALSE, 0);

    // ========== RIGHT SIDE ==========
    GtkWidget *right_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_grid_attach(GTK_GRID(main_grid), right_box, 1, 0, 2, 1);
    gtk_widget_set_hexpand(right_box, TRUE);
    gtk_widget_set_size_request(left_box, 400, -1); // width in pixels
    g_signal_connect(ui.input_submit, "clicked", G_CALLBACK(on_input_submit_clicked), NULL);

    // Dropdown + Info for selected process
    ui.proc_info.dropdown = gtk_combo_box_text_new();
    update_process_dropdown(ui.proc_info.dropdown);
    g_signal_connect(ui.proc_info.dropdown, "changed", G_CALLBACK(on_proc_dropdown_changed), NULL);
    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.dropdown, FALSE, FALSE, 0);

    ui.proc_info.pid_label = gtk_label_new("PID:");
    ui.proc_info.state_label = gtk_label_new("State:");
    ui.proc_info.priority_label = gtk_label_new("Priority:");
    ui.proc_info.mem_start_label = gtk_label_new("Mem Start:");
    ui.proc_info.mem_end_label = gtk_label_new("Mem End:");
    ui.proc_info.pc_label = gtk_label_new("PC:");

    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.pid_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.state_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.priority_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.mem_start_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.mem_end_label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(right_box), ui.proc_info.pc_label, FALSE, FALSE, 0);

    // Console
    GtkWidget *console_frame = gtk_frame_new("Console");
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, 400, 300);
    GtkWidget *textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), textview);
    gtk_container_add(GTK_CONTAINER(console_frame), scrolled);
    gtk_box_pack_start(GTK_BOX(right_box), console_frame, TRUE, TRUE, 0);
    ui.console_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

    // Memory Table (moved to right)
    GtkWidget *mem_frame = gtk_frame_new("Memory");
    GtkWidget *mem_scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(mem_scroll, 400, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(mem_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    ui.memory_table = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(ui.memory_table), 10);
    gtk_container_add(GTK_CONTAINER(mem_scroll), ui.memory_table);
    gtk_container_add(GTK_CONTAINER(mem_frame), mem_scroll);
    gtk_box_pack_start(GTK_BOX(right_box), mem_frame, TRUE, TRUE, 0);

    GtkWidget *mem_title1 = gtk_label_new("Name");
    GtkWidget *mem_title2 = gtk_label_new("Data");
    gtk_grid_attach(GTK_GRID(ui.memory_table), mem_title1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(ui.memory_table), mem_title2, 1, 0, 1, 1);

    for (int i = 0; i < 60; i++)
    {
        GtkWidget *label1 = gtk_label_new(memory[i].Name);
        GtkWidget *label2 = gtk_label_new(memory[i].Data);
        gtk_grid_attach(GTK_GRID(ui.memory_table), label1, 0, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(ui.memory_table), label2, 1, i + 1, 1, 1);
    }

    // Control Buttons
    GtkWidget *btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    const char *labels[] = {"Start", "Stop", "Step", "Finish", "Reset"};
    GCallback callbacks[] = {
        G_CALLBACK(on_start_clicked),
        G_CALLBACK(on_stop_clicked),
        G_CALLBACK(on_step_clicked),
        G_CALLBACK(on_finish_clicked),
        G_CALLBACK(on_reset_clicked)};
    btn_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);

    btn_start = gtk_button_new_with_label("Start");
    btn_stop = gtk_button_new_with_label("Stop");
    btn_step = gtk_button_new_with_label("Step");
    btn_finish = gtk_button_new_with_label("Finish");
    btn_reset = gtk_button_new_with_label("Reset");

    g_signal_connect(btn_start, "clicked", G_CALLBACK(on_start_clicked), NULL);
    g_signal_connect(btn_stop, "clicked", G_CALLBACK(on_stop_clicked), NULL);
    g_signal_connect(btn_step, "clicked", G_CALLBACK(on_step_clicked), NULL);
    g_signal_connect(btn_finish, "clicked", G_CALLBACK(on_finish_clicked), NULL);
    g_signal_connect(btn_reset, "clicked", G_CALLBACK(on_reset_clicked), NULL);

    gtk_box_pack_start(GTK_BOX(btn_box), btn_start, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_stop, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_step, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_finish, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(btn_box), btn_reset, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(right_box), btn_box, FALSE, FALSE, 0);

    if (pipe(pipe_fd) == 0)
    {
        // Redirect stdout to the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        setvbuf(stdout, NULL, _IOLBF, 0);
        fcntl(pipe_fd[1], F_SETFL, O_NONBLOCK);

        // Start thread to read pipe and push to GtkTextBuffer
        console_thread = g_thread_new("console_reader", console_output_thread, NULL);
    }

    // Start simulation thread
    int *params = g_new(int, 2);
    params[0] = algorithm;
    params[1] = quantum;
    sim_thread = g_thread_try_new("simulation", run_simulation, params, NULL);

    set_buttons_state(STATE_INIT);
    gtk_widget_show_all(main_window);
    start_gui_updater();
}

static void fill_queue_grid(GtkWidget *grid, int arr[], int size)
{
    GList *children = gtk_container_get_children(GTK_CONTAINER(grid));
    for (GList *l = children; l != NULL; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    for (int i = 0; i < size; i++)
    {
        if (arr[i] <= 0)
            continue;
        char cell[10];
        sprintf(cell, "%d", arr[i]);
        GtkWidget *label = gtk_label_new(cell);
        gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
        if (i < size - 1)
        {
            GtkWidget *sep = gtk_label_new(" | ");
            gtk_box_pack_start(GTK_BOX(box), sep, FALSE, FALSE, 0);
        }
    }
    gtk_grid_attach(GTK_GRID(grid), box, 0, 0, 1, 1);
    gtk_widget_show_all(grid);
}

static void update_ready_queues()
{
    const char *algo = getCurrentAlgo();
    Queue q;
    int arr[5];

    // Clear all queues first
    for (int i = 0; i < 4; i++)
    {
        int empty_row[1] = {0};
        fill_queue_grid(ui.ready_queues[i], empty_row, 0);
    }

    if (strcmp(algo, "FCFS") == 0)
    {
        q = getFCFSQ();
        int i = 0;
        Queue copy;
        initQueue(&copy);
        while (!isEmpty(&q) && i < 5)
        {
            int pid = dequeue(&q);
            arr[i++] = pid;
            enqueue(&copy, pid);
        }
        fill_queue_grid(ui.ready_queues[0], arr, i);
    }
    else if (strcmp(algo, "RR") == 0)
    {
        q = getRRQ();
        int i = 0;
        Queue copy;
        initQueue(&copy);
        while (!isEmpty(&q) && i < 5)
        {
            int pid = dequeue(&q);
            arr[i++] = pid;
            enqueue(&copy, pid);
        }
        fill_queue_grid(ui.ready_queues[0], arr, i);
    }
    else if (strcmp(algo, "MLFQ") == 0)
    {
        Queue levels[4] = {getMLF1Q(), getMLF2Q(), getMLF3Q(), getMLF4Q()};
        for (int l = 0; l < 4; l++)
        {
            int row[5], i = 0;
            Queue copy;
            initQueue(&copy);
            while (!isEmpty(&levels[l]) && i < 5)
            {
                int pid = dequeue(&levels[l]);
                row[i++] = pid;
                enqueue(&copy, pid);
            }
            fill_queue_grid(ui.ready_queues[l], row, i);
        }
    }
}

static void update_gui()
{

    // Update Process Count
    char info[256];
    sprintf(info, "Process Count: %d", getProcessCount());
    gtk_label_set_text(GTK_LABEL(ui.proc_count_label), info);

    // Update clock
    char temp[256];
    sprintf(temp, "Clock Cycle: %d", getClockCycles());
    gtk_label_set_text(GTK_LABEL(ui.clock_label), temp);

    // 2. Algorithm label
    const char *mode = getCurrentAlgo();
    sprintf(temp, "Algorithm: %s", (mode && strlen(mode) > 0) ? mode : "[Not Set]");
    gtk_label_set_text(GTK_LABEL(algo_label), temp);

    // 3. Ready Queues
    update_ready_queues();

    // Update selected process info
    update_process_dropdown(ui.proc_info.dropdown);
    int sel = gtk_combo_box_get_active(GTK_COMBO_BOX(ui.proc_info.dropdown));
    bool valid = (sel >= 0 && sel < getProcessCount());
    if (valid)
    {
        int index = sel + 1;
        sprintf(temp, "PID: %d", index);
        gtk_label_set_text(GTK_LABEL(ui.proc_info.pid_label), temp);

        char state[50] = "";
        getState(index, state);
        sprintf(temp, "State: %s", state);
        gtk_label_set_text(GTK_LABEL(ui.proc_info.state_label), temp);

        sprintf(temp, "Priority: %d", getPri(index));
        gtk_label_set_text(GTK_LABEL(ui.proc_info.priority_label), temp);

        sprintf(temp, "Mem Start: %d", getMemStart(index));
        gtk_label_set_text(GTK_LABEL(ui.proc_info.mem_start_label), temp);

        sprintf(temp, "Mem End: %d", getMemEnd(index));
        gtk_label_set_text(GTK_LABEL(ui.proc_info.mem_end_label), temp);

        sprintf(temp, "PC: %d", getPC(index));
        gtk_label_set_text(GTK_LABEL(ui.proc_info.pc_label), temp);
    }

    // Blocked queue
    Queue qtemp = getBlockedQ();
    int blocked[5], bi = 0;
    Queue copy;
    initQueue(&copy);
    while (!isEmpty(&qtemp) && bi < 5)
    {
        int pid = dequeue(&qtemp);
        blocked[bi++] = pid;
        enqueue(&copy, pid);
    }
    fill_queue_grid(ui.blocked_queue, blocked, bi);

    // Memory
    MemoryWord *mem = getMemory();
    GList *children = gtk_container_get_children(GTK_CONTAINER(ui.memory_table));
    for (GList *l = children; l != NULL; l = l->next)
        gtk_widget_destroy(GTK_WIDGET(l->data));
    g_list_free(children);

    // Add headers
    GtkWidget *header1 = gtk_label_new("Name");
    GtkWidget *header2 = gtk_label_new("Data");
    gtk_grid_attach(GTK_GRID(ui.memory_table), header1, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(ui.memory_table), header2, 1, 0, 1, 1);

    // Populate rows
    for (int i = 0; i < 60; i++)
    {
        if (strlen(mem[i].Name) == 0 && strlen(mem[i].Data) == 0)
            continue;

        GtkWidget *label1 = gtk_label_new(mem[i].Name);
        GtkWidget *label2 = gtk_label_new(mem[i].Data);
        gtk_grid_attach(GTK_GRID(ui.memory_table), label1, 0, i + 1, 1, 1);
        gtk_grid_attach(GTK_GRID(ui.memory_table), label2, 1, i + 1, 1, 1);
    }
    gtk_widget_show_all(ui.memory_table);

    // Running process info
    int pid = getRunningPID();
    sprintf(temp, "PID: %d", pid);
    gtk_label_set_text(GTK_LABEL(ui.running_pid_label), temp);

    char *inst = getCurInst(pid);
    sprintf(temp, "Instruction: %s", inst ? inst : "");
    gtk_label_set_text(GTK_LABEL(ui.cur_inst_label), temp);

    sprintf(temp, "Time In Queue: %d", getTimeInQueue(pid));
    gtk_label_set_text(GTK_LABEL(ui.time_in_queue_label), temp);

    // Mutexes
    Queue (*getQ[3])() = {getUserInputQ, getUserOnputQ, getFileQ};
    int (*getStatus[3])() = {getUserInputStatus, getUserOnputStatus, getFileStatus};
    for (int m = 0; m < 3; m++)
    {
        Queue qx = getQ[m]();
        int vals[5], vi = 0;
        Queue tempQ;
        initQueue(&tempQ);
        while (!isEmpty(&qx) && vi < 5)
        {
            int v = dequeue(&qx);
            vals[vi++] = v;
            enqueue(&tempQ, v);
        }
        fill_queue_grid(ui.mutex_tables[m], vals, vi);
        int stat = getStatus[m]();
        if (stat == -1)
            gtk_label_set_text(GTK_LABEL(ui.mutex_states[m]), "State: Free");
        else
        {
            sprintf(temp, "State: Locked by %d", stat);
            gtk_label_set_text(GTK_LABEL(ui.mutex_states[m]), temp);
        }
    }

    if (waitingForInput)
    {
        gtk_widget_set_sensitive(ui.input_entry, TRUE);
        gtk_widget_set_sensitive(ui.input_submit, TRUE);
    }
    else
    {
        gtk_widget_set_sensitive(ui.input_entry, FALSE);
        gtk_widget_set_sensitive(ui.input_submit, FALSE);
    }
}

static void on_file_clicked(GtkButton *button, gpointer user_data)
{
    GtkWidget *dialog = gtk_file_chooser_dialog_new(
        "Select Program File",
        GTK_WINDOW(user_data),
        GTK_FILE_CHOOSER_ACTION_OPEN,
        "_Cancel", GTK_RESPONSE_CANCEL,
        "_Open", GTK_RESPONSE_ACCEPT,
        NULL);

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_set_name(filter, "Text Files");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);

    if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        gtk_entry_set_text(GTK_ENTRY(file_entry), filename);
        g_free(filename);
    }
    gtk_widget_destroy(dialog);
}

static void on_algo_changed(GtkComboBox *combo, gpointer user_data)
{
    gboolean sensitive = (gtk_combo_box_get_active(combo) == 1); // RR is index 1
    gtk_widget_set_sensitive(quantum_spin, sensitive);
    gtk_widget_set_sensitive(quantum_label, sensitive);
}

static void on_load_clicked(GtkButton *button, gpointer user_data)
{
    const gchar *filename = gtk_entry_get_text(GTK_ENTRY(file_entry));
    gint arrival_time = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(arrival_spin));

    gtk_label_set_text(GTK_LABEL(status_label), "");
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(instructions_textview));
    gtk_text_buffer_set_text(buffer, "", -1);

    if (strlen(filename) > 0 && arrival_time >= 0)
    {
        FILE *file = fopen(filename, "r");
        if (file)
        {
            char file_content[1024] = {0};
            size_t len = fread(file_content, 1, sizeof(file_content) - 1, file);
            fclose(file);

            if (len > 0)
            {
                gtk_label_set_text(GTK_LABEL(status_label), "Program loaded successfully!");
                gtk_text_buffer_set_text(buffer, file_content, len);
                readProcessAndStore((char *)filename, arrival_time);
            }
            else
            {
                gtk_label_set_text(GTK_LABEL(status_label), "Error: Empty file!");
            }
        }
        else
        {
            gtk_label_set_text(GTK_LABEL(status_label), "Error: Failed to load file!");
        }
    }
    else
    {
        gtk_label_set_text(GTK_LABEL(status_label), "Error: Invalid input!");
    }
}

static void on_run_clicked(GtkButton *button, gpointer user_data)
{
    GtkWindow *window = GTK_WINDOW(user_data);
    int algorithm = gtk_combo_box_get_active(GTK_COMBO_BOX(algo_combo)) + 1;
    int quantum = 0;

    if (algorithm == 2)
    { // RR
        quantum = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(quantum_spin));
    }

    create_simulation_window(algorithm, quantum);
    gtk_widget_hide(GTK_WIDGET(window));
}

static void activate(GtkApplication *app, gpointer user_data)
{
    GtkWidget *window = gtk_application_window_new(app);
    starter_window = window;
    gtk_window_set_title(GTK_WINDOW(window), "Scheduler Loader");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 400);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    GtkWidget *grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_container_add(GTK_CONTAINER(window), grid);

    // File selection
    GtkWidget *file_button = gtk_button_new_with_label("Browse");
    g_signal_connect(file_button, "clicked", G_CALLBACK(on_file_clicked), window);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Select Program File:"), 0, 0, 1, 1);
    file_entry = gtk_entry_new();
    gtk_editable_set_editable(GTK_EDITABLE(file_entry), FALSE);
    gtk_grid_attach(GTK_GRID(grid), file_entry, 1, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), file_button, 2, 0, 1, 1);

    // Arrival time
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Arrival Time:"), 0, 1, 1, 1);
    arrival_spin = gtk_spin_button_new_with_range(0, 1000, 1);
    gtk_grid_attach(GTK_GRID(grid), arrival_spin, 1, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_button_new_with_label("Load"), 2, 1, 1, 1);

    // Algorithm selection
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Scheduling Algorithm:"), 0, 2, 1, 1);
    algo_combo = gtk_combo_box_text_new();
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo), "FCFS");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo), "Round Robin");
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(algo_combo), "MLFQ");
    gtk_combo_box_set_active(GTK_COMBO_BOX(algo_combo), 0);
    g_signal_connect(algo_combo, "changed", G_CALLBACK(on_algo_changed), NULL);
    gtk_grid_attach(GTK_GRID(grid), algo_combo, 1, 2, 2, 1);

    // Quantum (only for RR)
    quantum_label = gtk_label_new("Quantum:");
    gtk_grid_attach(GTK_GRID(grid), quantum_label, 0, 3, 1, 1);
    quantum_spin = gtk_spin_button_new_with_range(1, 100, 1);
    gtk_widget_set_sensitive(quantum_spin, FALSE);
    gtk_grid_attach(GTK_GRID(grid), quantum_spin, 1, 3, 1, 1);

    // Run button
    gtk_grid_attach(GTK_GRID(grid), gtk_button_new_with_label("Run"), 2, 3, 1, 1);

    // Status label
    status_label = gtk_label_new("");
    gtk_grid_attach(GTK_GRID(grid), status_label, 0, 4, 3, 1);

    // Instructions display
    GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
    gtk_widget_set_size_request(scrolled, 600, 300);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled),
                                   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    instructions_textview = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(instructions_textview), FALSE);
    gtk_container_add(GTK_CONTAINER(scrolled), instructions_textview);
    gtk_grid_attach(GTK_GRID(grid), scrolled, 0, 5, 3, 1);

    // Connect signals
    GtkWidget *load_button = gtk_grid_get_child_at(GTK_GRID(grid), 2, 1);
    g_signal_connect(load_button, "clicked", G_CALLBACK(on_load_clicked), NULL);

    GtkWidget *run_button = gtk_grid_get_child_at(GTK_GRID(grid), 2, 3);
    g_signal_connect(run_button, "clicked", G_CALLBACK(on_run_clicked), window);

    gtk_widget_show_all(window);
}

int main(int argc, char **argv)
{
    GtkApplication *app = gtk_application_new("org.gtk.scheduler", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(activate), NULL);
    int status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    return status;
}

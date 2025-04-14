#include <stdio.h>
#include <stdlib.h>

#define MAX_SIZE 5

// Queue structure
typedef struct {
    int items[MAX_SIZE];
    int front, rear;
} Queue;

// Initialize the queue
void initQueue(Queue* q) {
    q->front = -1;
    q->rear = -1;
}

// Check if the queue is full
int isFull(Queue* q) {
    return q->rear == MAX_SIZE - 1;
}

// Check if the queue is empty
int isEmpty(Queue* q) {
    return q->front == -1;
}

// Enqueue (add an element)
void enqueue(Queue* q, int value) {
    if (isFull(q)) {
        printf("Queue is full\n");
        return;
    }
    if (q->front == -1) {
        q->front = 0;  // If queue is empty, set front to 0
    }
    q->rear++;
    q->items[q->rear] = value;
}

// Dequeue (remove an element)
int dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return -1;  // Return -1 if the queue is empty
    }
    int value = q->items[q->front];
    q->front++;
    if (q->front > q->rear) {  // Reset queue when it's empty
        q->front = q->rear = -1;
    }
    return value;
}

// Peek (view the front element)
int peek(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return -1;
    }
    return q->items[q->front];
}

int main() {
    Queue q;
    initQueue(&q);

    enqueue(&q, 10);
    enqueue(&q, 20);
    enqueue(&q, 30);

    printf("Dequeued: %d\n", dequeue(&q));
    printf("Front element: %d\n", peek(&q));

    return 0;
}

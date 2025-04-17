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
// Modify enqueue and dequeue for circular behavior:
void enqueue(Queue* q, int value) {
    if ((q->rear + 1) % MAX_SIZE == q->front) {  // Check if full
        printf("Queue is full\n");
        return;
    }
    if (isEmpty(q)) {
        q->front = q->rear = 0;
    } else {
        q->rear = (q->rear + 1) % MAX_SIZE;  // Wrap around
    }
    q->items[q->rear] = value;
}

int dequeue(Queue* q) {
    if (isEmpty(q)) {
        printf("Queue is empty\n");
        return -1;
    }
    int value = q->items[q->front];
    if (q->front == q->rear) {
        q->front = q->rear = -1;  // Reset if last element
    } else {
        q->front = (q->front + 1) % MAX_SIZE;  // Wrap around
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

// int main() {
//     Queue q;
//     initQueue(&q);

//     enqueue(&q, 10);
//     enqueue(&q, 20);
//     enqueue(&q, 30);

//     printf("Dequeued: %d\n", dequeue(&q));
//     printf("Front element: %d\n", peek(&q));

//     return 0;
// }

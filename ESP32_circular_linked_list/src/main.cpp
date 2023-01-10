#include <Arduino.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

// Node structure for a circular linked list
struct node {
  int data;
  struct node *next;
};
node* currentNode;

// Function to create a circular linked list with a specified size
struct node* create_circular_linked_list(int size) {
  struct node *head, *current, *temp;

  // Create the first node
  head = (struct node*) malloc(sizeof(struct node));
  current = head;

  // Create the rest of the nodes
  for (int i = 1; i < size; i++) {
    temp = (struct node*) malloc(sizeof(struct node));
    current->next = temp;
    current = temp;
  }

  // Link the last node to the head to create the circular linked list
  current->next = head;

  return head;
}

// Function to populate a circular linked list with data from an array
void populate_circular_linked_list(struct node *head, int data[], int size) {
  struct node *current = head;

  // Assign values to the data field of each node
  for (int i = 0; i < size; i++) {
    current->data = data[i];
    current = current->next;
  }
}

node* printNext(node* node) {
   Serial.println(node->data);
   return node->next;
}

void setup() {
  Serial.begin(115200);
  delay(500);

  // Create a circular linked list with size 5
  currentNode = create_circular_linked_list(5);

  // Populate the linked list with data from an array
  int data[] = {1, 2, 3, 4, 5};
  populate_circular_linked_list(currentNode, data, 5);

  while (currentNode != NULL) {
    currentNode = printNext(currentNode);
    delay(1000);
  }

}

void loop() {
}

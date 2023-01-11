#include <Arduino.h>

boolean DEBUG = false;

// Node structure for a circular linked list.
struct node {
  int data;
  struct node *next;
};
struct node* currentNode;

/** 
 * Prints out the contents of the linked list 
 */
void printLinkedList(node* head){
  struct node *current = head;
  Serial.println("-----Linked List Contents-----");
  do {
    int value = current->data;
    Serial.println(String(value));
    current = current->next;
  } while (current != head);
}

/*
 * Returns a count of items in the linked list 
*/
int countCircularLinkedList(node* head) { 
    int count = 0;
    node* current = head;
    if (head == nullptr) return 0;
    do {
        current = current->next;
        count++;
    } while (current != head);
    return count;
}

/*
 * Creates a circular linked list of the given size and returns a pointer to the head node 
*/
struct node* createCircularLinkedList(int size) {  //TODO new version
    node* head = nullptr;
    node* tail = nullptr;

    for (int i = 0; i < size; i++) {
        node* newNode = new node;
        newNode->data = i;
        newNode->next = nullptr;

        if (head == nullptr) {
            head = newNode;
            tail = newNode;
        } else {
            tail->next = newNode;
            tail = newNode;
        }
    }

    // Make the list circular by connecting the last node to the head node
    tail->next = head;
    return head;
}

/**
 * @brief Populates the given circular linked list with one complete cycle of sinusoid data
 * 
 * Since we know the SAMPLES_PER_CYCLE of the waveform, we'll put that many values into the linked list.
 * The timer will call function onTimer() at the precise rate needed to produce the desired output frequency. 
 * 
 * @param head pointer to the circular linked list to be populated with one complete cycle of sinusoid data
 * @return void
 */
void populateCircularLinkedList(struct node *head) {
  struct node *current = head;
  int size = countCircularLinkedList(head);
  for (int i = 0; i < size; i++) {
    int value = i;
    current->data = value;
    current = current->next;
  }
  if (DEBUG){
    printLinkedList(head);
  }
}

void test(){
  Serial.println("test() creating a circular linked list with 10 elements.");
  node *head = createCircularLinkedList(10);
  printLinkedList(head);
  Serial.println("test() DONE");
}

void setup() {
  Serial.begin(115200);
  delay(500); //delay to give Serial.begin() time to finish
  test();
}

void loop() {
  // put your main code here, to run repeatedly:
}
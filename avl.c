#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

typedef struct Node {
    int key;
    struct Node *left;
    struct Node *right;
    int height;
} Node;

typedef struct AVLTree {
    Node *root;
    pthread_mutex_t lock;
} AVLTree;

Node* createNode(int key) {
    Node* newNode = (Node*)malloc(sizeof(Node));
    if (newNode) {
        newNode->key = key;
        newNode->left = newNode->right = NULL;
        newNode->height = 1;
    }
    return newNode;
}

int max(int a, int b) {
    return (a > b) ? a : b;
}

int height(Node *node) {
    if (node == NULL)
        return 0;
    return node->height;
}

int getBalance(Node *node) {
    if (node == NULL)
        return 0;
    return height(node->left) - height(node->right);
}

Node* rightRotate(Node* y) {
    Node* x = y->left;
    Node* T2 = x->right;

    x->right = y;
    y->left = T2;

    y->height = max(height(y->left), height(y->right)) + 1;
    x->height = max(height(x->left), height(x->right)) + 1;

    return x;
}

Node* leftRotate(Node* x) {
    Node* y = x->right;
    Node* T2 = y->left;

    y->left = x;
    x->right = T2;

    x->height = max(height(x->left), height(x->right)) + 1;
    y->height = max(height(y->left), height(y->right)) + 1;

    return y;
}

Node* insert(Node* node, int key) {
    if (node == NULL)
        return createNode(key);

    if (key < node->key)
        node->left = insert(node->left, key);
    else if (key > node->key)
        node->right = insert(node->right, key);
    else
        return node;

    node->height = 1 + max(height(node->left), height(node->right));

    int balance = getBalance(node);

    if (balance > 1 && key < node->left->key)
        return rightRotate(node);

    if (balance < -1 && key > node->right->key)
        return leftRotate(node);

    if (balance > 1 && key > node->left->key) {
        node->left = leftRotate(node->left);
        return rightRotate(node);
    }

    if (balance < -1 && key < node->right->key) {
        node->right = rightRotate(node->right);
        return leftRotate(node);
    }

    return node;
}

Node* search(Node* node, int key) {
    if (node == NULL || node->key == key)
        return node;

    if (key < node->key)
        return search(node->left, key);

    return search(node->right, key);
}

void preOrderTraversal(Node *root) {
    if (root != NULL) {
        printf("%d ", root->key);
        preOrderTraversal(root->left);
        preOrderTraversal(root->right);
    }
}

void inOrderTraversal(Node *root) {
    if (root != NULL) {
        inOrderTraversal(root->left);
        printf("%d ", root->key);
        inOrderTraversal(root->right);
    }
}

void* insertThread(void* arg) {
    AVLTree* avlTree = (AVLTree*)arg;
    int key;
    printf("Enter value to insert: ");
    scanf("%d", &key);

    //while(getchar()!='\n');

    pthread_mutex_lock(&avlTree->lock);
    avlTree->root = insert(avlTree->root, key);
    pthread_mutex_unlock(&avlTree->lock);
    return NULL;
}

void* searchThread(void* arg) {
    AVLTree* avlTree = (AVLTree*)arg;
    int key;
    printf("Enter value to check: ");
    scanf("%d", &key);

    //while(getchar()!='\n');

    pthread_mutex_lock(&avlTree->lock);
    Node* found = search(avlTree->root, key);
    if (found != NULL)
        printf("Key found: %d\n", found->key);
    else
        printf("Key not found\n");
    pthread_mutex_unlock(&avlTree->lock);
    return NULL;
}

void* inOrderTraversalThread(void* arg) {
    AVLTree* avlTree = (AVLTree*)arg;
    pthread_mutex_lock(&avlTree->lock);
    printf("In-order traversal: ");
    inOrderTraversal(avlTree->root);
    printf("\n");
    pthread_mutex_unlock(&avlTree->lock);
    return NULL;
}

Node* minValueNode(Node* node) {
    Node* current = node;
    while (current && current->left != NULL)
        current = current->left;
    return current;
}

Node* deleteNode(Node* root, int key) {
    if (root == NULL)
        return root;

    if (key < root->key)
        root->left = deleteNode(root->left, key);
    else if (key > root->key)
        root->right = deleteNode(root->right, key);
    else {
        if (root->left == NULL || root->right == NULL) {
            Node* temp = root->left ? root->left : root->right;
            if (temp == NULL) {
                temp = root;
                root = NULL;
            } else
                *root = *temp;
            free(temp);
        } else {
            Node* temp = minValueNode(root->right);
            root->key = temp->key;
            root->right = deleteNode(root->right, temp->key);
        }
    }

    if (root == NULL)
        return root;

    root->height = 1 + max(height(root->left), height(root->right));

    int balance = getBalance(root);

    if (balance > 1 && getBalance(root->left) >= 0)
        return rightRotate(root);

    if (balance > 1 && getBalance(root->left) < 0) {
        root->left = leftRotate(root->left);
        return rightRotate(root);
    }

    if (balance < -1 && getBalance(root->right) <= 0)
        return leftRotate(root);

    if (balance < -1 && getBalance(root->right) > 0) {
        root->right = rightRotate(root->right);
        return leftRotate(root);
    }

    return root;
}

void* deleteThread(void* arg) {
    AVLTree* avlTree = (AVLTree*)arg;
    int key;
    printf("Enter value to delete: ");
    scanf("%d", &key);

    //while(getchar()!='\n');

    pthread_mutex_lock(&avlTree->lock);
    avlTree->root = deleteNode(avlTree->root, key);
    pthread_mutex_unlock(&avlTree->lock);
    return NULL;
}

#define MAX_COMMANDS 100
typedef struct {
    char command[20];
    int key;
} UserCommand;

void takeUserInput(AVLTree *avlTree, UserCommand *commands, int *numCommands) {
    char input[50];
    *numCommands = 0;

    while (1) {
        printf("Enter command (insert/delete/contains/inorder/preorder) followed by a key or type 'exit' to stop: ");
        fgets(input, 50, stdin);

        if (strncmp(input, "exit", 4) == 0) {
            break;
        }

        sscanf(input, "%s %d", commands[*numCommands].command, &commands[*numCommands].key);
        (*numCommands)++;
    }
}

void executeCommands(AVLTree *avlTree, UserCommand *commands, int numCommands) {
    for (int i = 0; i < numCommands; i++) {
        if (strcmp(commands[i].command, "insert") == 0) {
            pthread_mutex_lock(&avlTree->lock);
            avlTree->root = insert(avlTree->root, commands[i].key);
            pthread_mutex_unlock(&avlTree->lock);
        } else if (strcmp(commands[i].command, "delete") == 0) {
            pthread_mutex_lock(&avlTree->lock);
            avlTree->root = deleteNode(avlTree->root, commands[i].key);
            pthread_mutex_unlock(&avlTree->lock);
        } else if (strcmp(commands[i].command, "contains") == 0) {
            pthread_mutex_lock(&avlTree->lock);
            Node *found = search(avlTree->root, commands[i].key);
            if (found != NULL)
                printf("Key found: %d\n", found->key);
            else
                printf("Key not found\n");
            pthread_mutex_unlock(&avlTree->lock);
        }
    }
}

int main() {
    AVLTree avlTree;
    avlTree.root = NULL;
    pthread_mutex_init(&avlTree.lock, NULL);

    UserCommand commands[MAX_COMMANDS];
    int numCommands;

    takeUserInput(&avlTree, commands, &numCommands); 
    executeCommands(&avlTree, commands, numCommands);

    printf("In-order traversal: ");
    inOrderTraversal(avlTree.root);
    printf("\n");

    printf("Pre-order traversal: ");
    preOrderTraversal(avlTree.root);
    printf("\n");

    pthread_mutex_destroy(&avlTree.lock);

    return 0;
}






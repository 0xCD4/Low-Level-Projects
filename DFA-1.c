#include <stdio.h>
#include <string.h>

// Define the states
typedef enum { Q0, Q1, Q2, Q3, Q4 } State;

// DFA transition function
State transition(State current_state, char input) {
    switch (current_state) {
        case Q0:
            if (input == 'x') return Q1;  // From q0 to q1 on input 'x'
            if (input == 'y') return Q3;  // From q0 to q3 on input 'y'
            break;
        case Q1:
            if (input == 'x' || input == 'y') return Q2;  // From q1 to q2 on input 'x' or 'y'
            break;
        case Q2:
            if (input == 'x' || input == 'y') return Q1;  // From q2 to q1 on input 'x' or 'y'
            break;
        case Q3:
            if (input == 'x' || input == 'y') return Q4;  // From q3 to q4 on input 'x' or 'y'
            break;
        case Q4:
            if (input == 'x' || input == 'y') return Q3;  // From q4 to q3 on input 'x' or 'y'
            break;
    }
    return current_state;  // Invalid transition
}

// DFA acceptance function
int is_accepted(const char* input) {
    State current_state = Q0;  // Start at initial state q0

    for (int i = 0; i < strlen(input); ++i) {
        current_state = transition(current_state, input[i]);
    }

    // Accept if the current state is q2 or q4
    return (current_state == Q1 || current_state == Q4);
}

int main() {
    char input[100];

    printf("Enter a string (consisting of 'x' and 'y'): ");
    scanf("%s", input);

    
    for (int i = 0; i < strlen(input); ++i) {
        if (input[i] != 'x' && input[i] != 'y') {
            printf("Invalid input. Only 'x' and 'y' are allowed.\n");
            return 1;
        }
    }

  
    if (is_accepted(input)) {
        printf("The input string is accepted by the DFA.\n");
    } else {
        printf("The input string is rejected by the DFA.\n");
    }

    return 0;
}

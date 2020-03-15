from hw2_util import Node, Graph, consistent, constraint
import random

def select_value_fc(prev_path, domain, i):
    # Primary variable domain copy
    domain_i = domain[i]

    # Save current domain for reset should children value is not found
    curr_domain = [x[:] for x in domain]

    # Initialize variable to check if the entire children domain is valid
    domain_invalid = False

    while len(domain_i) > 0:
        # Get primary variable value
        alpha_assignment = domain_i.pop()

        # Loop through children domain
        domain_k = []
        for k in range(i + 1, len(domain)):
            domain_k = domain[k]
            for value in domain_k.copy():
                new_path = prev_path.copy()
                new_path.extend([alpha_assignment, value])
                if not constraint(i,k,alpha_assignment,value):
                    domain_k.remove(value)

            # If children k domain is empty, signal the need to reset primary variable
            if len(domain_k) == 0:
                domain[i+1:] = [x[:] for x in curr_domain[i+1:]]
                domain_invalid = True
                break;
            else:
                domain_invalid = False

        # Check if reached here because of invalid domain or a valid variable,
        # if invalid domain, go back to repick primary variable
        if not domain_invalid:
            return alpha_assignment
    return None

def domain_reset(domain, original_copy, start_idx):
    # Deep copy of reset domain
    domain[start_idx:] = [x[:] for x in original_copy[start_idx:]]

def bt_fc(queen_count=4):
    # Construct Graph
    chess_board = Graph(queen_count)
    # Domain Value in manipulation
    domain_list = [chess_board.getNode(
        x).get_possible_val().copy() for x in range(queen_count)]
    # Book keeping Domain
    book_keeping_domain_list = [None] * queen_count
    visited_state = 0

    i = 0
    path = []
    while 0 <= i and i < queen_count:

        # Save domain before it is manipulated
        if book_keeping_domain_list[i] is None:
            book_keeping_domain_list[i] = [x[:] for x in domain_list]

        # Select Value
        x_i = select_value_fc(path, domain_list, i)
        if x_i is None:
            # If dead end, reset domain
            domain_reset(domain_list, book_keeping_domain_list[i - 1], i)

            # Set book keeping domain back to None
            # as if it has NEVER been reached
            book_keeping_domain_list[i] = None

            # Pop the path and go back one level to be reassigned
            if len(path) != 0:
                path.pop()
            i -= 1
        else:
            i += 1
            # Add to path
            path.append(x_i)
            visited_state += 1
    if i == 0:
        return "The problem is inconsistent"
    else:
        return path
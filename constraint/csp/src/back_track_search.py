from hw2_util import Node, Graph, consistent
import random

def select_value(prev_path, domain):

    while len(domain) > 0:
        # Randomly select a value
        alpha_assignment = domain.pop()

        # Construct a path
        new_path = prev_path.copy()
        new_path.append(alpha_assignment)

        # Check if consistent
        if consistent(new_path):
            return alpha_assignment
    return None

def back_track_search(queen_count=4):
    chess_board = Graph(queen_count)
    domain_list = [list() for x in range(queen_count)]
    visited_states = 0

    i = 1
    path = []
    domain_list[i - 1] = chess_board.getNode(i - 1).get_possible_val().copy()

    while 1 <= i and i <= queen_count:

        # Select Value
        x_i = select_value(path, domain_list[i - 1])
        if x_i is None:
            # Dead end
            i -= 1
            # Remove the end value for path, goes back to re-assign it
            if len(path) != 0:
                path.pop()
        else:
            # Valid Possibility
            i += 1
            # Add to path
            path.append(x_i)
            visited_states += 1
            
            # Update domain 
            if i <= queen_count:
                domain_list[i - 1] = chess_board.getNode(i - 1).get_possible_val().copy()
    if i == 0:
        return "The problem is inconsistent"
    else:
        return path
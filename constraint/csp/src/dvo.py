from hw2_util import Node, Queen, consistent, constraint
import random
import copy

def select_value_fc(prev_path, domain, i):
    # Retrieve Variable
    primary_queen = domain[i]
    # Signal that the variable value has been set
    primary_queen.setVal()

    # save domain in case a backup is required
    curr_domain = copy.deepcopy(domain)

    # Initialize variable to check if the entire children domain is valid
    domain_invalid = False
    best_value_selected = False

    min_alpha_constrained = len(domain) ** 2
    min_alpha_value = -1
    viable_alpha_values = []

    while len(primary_queen.getDomain()) > 0:
        # Get value 
        alpha_assignment = primary_queen.getDomain().pop()
        this_alpha_constrained = 0
        # Loop through un assigned variables that are behind 
        domain_k = []
        for k in range(i + 1, len(domain)):
            
            # Get values 
            domain_k = domain[k]
            for value in domain_k.getDomain().copy():
                # Check if the new assignment is valid
                new_path = prev_path.copy()
                new_path.extend([alpha_assignment, value])
                if not constraint(primary_queen.getIdx(), domain_k.getIdx(), alpha_assignment, value):
                    domain_k.getDomain().remove(value)
                    this_alpha_constrained += 1

            # Signal the flag if a domain is invalid and a primary variable re-assignment is required
            if len(domain_k.getDomain()) == 0:
                domain[i+1:] = copy.deepcopy(curr_domain[i+1:])
                domain_invalid = True
                break
            else:
                domain_invalid = False
        
        if not best_value_selected:
            if this_alpha_constrained <= min_alpha_constrained:
                min_alpha_value = alpha_assignment

            viable_alpha_values.append(alpha_assignment)

            if len(primary_queen.getDomain()) == 0:
                best_value_selected = True
                viable_alpha_values.remove(min_alpha_value)
                primary_queen.getDomain().append(min_alpha_value)

            # restore the stuff
            domain[i+1:] = copy.deepcopy(curr_domain[i+1:])
        else:
            # Check to return or reassign
            if not domain_invalid:
                primary_queen.getDomain().extend(viable_alpha_values)
                return alpha_assignment
    return None

def rearrange(variable_list, i, reversed = False):
    
    # Sort the variable list so that smallest/largest domain goes first.
    # Assigned variables are always before i
    min_val = len(variable_list)
    min_idx = -1
    variable_list[i+1:] = sorted(variable_list[i+1:], key=lambda x: x.getDomainSize(), reverse = reversed)
    return variable_list

def dvo(queen_count=4, least = True):
    # Construct a list of variables
    chess_board = list()
    for count in range(1,queen_count + 1):
        chess_board.append(Queen(count,queen_count))
    
    # Initialize book keeping
    book_keeping_domain_list = [None] * queen_count
    visited_state = 0

    i = 0
    path = [None] * queen_count
    # No rearrange here since all domains have the same size
    while 0 <= i and i < queen_count:
        
        # initilize book keeping
        if book_keeping_domain_list[chess_board[i].getIdx() - 1] is None:
            book_keeping_domain_list[chess_board[i].getIdx() - 1] = copy.deepcopy(
                chess_board)

        x_i = select_value_fc(path, chess_board, i)

        if x_i is None:
            # If is dead end
            # take note of the item to be removed from book-keeping list, since it is invalidated before
            # it can be used 
            removal_idx = chess_board[i].getIdx()
            
            # Restore domain to the chessboard
            chess_board = copy.deepcopy(
                book_keeping_domain_list[chess_board[i - 1].getIdx() - 1])
            # Wish recursion could be used, book keeping would be a lot easier and dont have to spend 20hr on this

            # Check if the removal index is always valid, tho it should always pass
            if (removal_idx - 1 < len(book_keeping_domain_list)):
                # removal idx is subtracted by one because Queen index from 1 to n and 
                # python list from 0
                book_keeping_domain_list[removal_idx - 1] = None

            # Reset the last variable of the path, to be populated again
            if len(path) != 0:
                path[chess_board[i].getIdx() - 1] = None
            i -= 1
        else:
            # Update book keeping value so that the value assigned will not be used again, until a higher 
            # variable re-assignment is called.
            book_keeping_domain_list[chess_board[i].getIdx() - 1][i] = copy.deepcopy(chess_board[i])
            if i < queen_count - 1:
                
                # Set the value of x_i to the correct position of the path
                path[chess_board[i].getIdx() - 1] = x_i

                # rearrange the variables that has yet to be assigned to be sorted by their domain size
                some_var_not_wrong = rearrange(chess_board,i, least)
                i += 1
                chess_board = some_var_not_wrong
            else:
                # If this is the end variable, dont sort anymore
                path[chess_board[i].getIdx() - 1] = x_i
                i += 1
            visited_state += 1
    if i == 0:
        return "The problem is inconsistent"
    else:
        return path
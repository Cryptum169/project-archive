import random


class Node:
    def __init__(self, dataval=None):
        self.dataval = [x for x in range(1, dataval+1)]
        self.rand_domain = self.dataval.copy()
        random.shuffle(self.rand_domain)
        self.nextval = None
        self.nextNode = None

    def set_child(self, nextNode):
        self.nextNode = nextNode

    def get_child_ptr(self):
        return self.nextNode

    def get_possible_val(self):
        return self.dataval


class Graph:
    def __init__(self, node_num=1):
        self.nodeList = [Node(node_num) for x in range(node_num)]

    def get_head(self):
        return self.nodeList[0]

    def getNode(self, idx):
        return self.nodeList[idx]

class Queen:
    def __init__(self, idx, chessboard_size = 4):
        self.idx = idx
        self.position = [x for x in range(1, chessboard_size + 1)]
        self.assigned = False

    def getDomain(self):
        return self.position
    
    def getDomainSize(self):
        return len(self.position)

    def getIdx(self):
        return self.idx

    def setVal(self):
        self.assigned = True

    def getStatus(self):
        return self.assigned

def constraint(i, j, val_i, val_j):
    return val_i != val_j and abs(i - j) != abs(val_i - val_j)

def consistent(queen_pose):
    num_of_queen = len(queen_pose)
    for first_queen in range(num_of_queen):
        for second_queen in range(first_queen + 1, num_of_queen):
            arc_consis = constraint(first_queen, second_queen, queen_pose[
                first_queen], queen_pose[second_queen])
            if arc_consis is False:
                return False
    return True

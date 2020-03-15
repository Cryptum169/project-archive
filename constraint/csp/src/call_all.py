import signal
import sys
import time

from standard_search import standard_search
from back_track_search import back_track_search
from bt_fc import bt_fc
from dvo import dvo

def main(mode='s'):
    def handler(signum, frame):
        raise Exception("10 min time reached")

    def test_timeout(search):
        print("Time out search started")
        signal.signal(signal.SIGALRM, handler)
        queen_count = 4

        timeout_val = 0
        continued_count = 0
        while True:
            signal.alarm(600)
            try:
                start = time.time()
                search(queen_count)
                end = time.time()
                continued_count = 0
                if mode == 's':
                    print("{} queens take {} to run".format(
                        queen_count, end-start))
                else:
                    print("({},{})".format(queen_count, end-start), end="")
            except:
                print("\nTime out on {} queens".format(queen_count))
                # break
                continued_count += 1
                if continued_count == 5:
                    break
            queen_count += 1

    def test_timeout_dvo_lc(search):
        print("Time out search started")
        signal.signal(signal.SIGALRM, handler)
        queen_count = 4

        timeout_val = 0
        continued_count = 0
        while True:
            signal.alarm(600)
            try:
                start = time.time()
                search(queen_count, True)
                end = time.time()
                continued_count = 0
                if mode == 's':
                    print("{} queens take {} to run".format(
                        queen_count, end-start))
                else:
                    print("({},{})".format(queen_count, end-start), end="")
            except:
                print("\nTime out on {} queens".format(queen_count))
                # break
                continued_count += 1
                if continued_count == 5:
                    break
            queen_count += 1

    def dvo_test_timeout(search, least=True):
        print("Time out search started")
        signal.signal(signal.SIGALRM, handler)
        queen_count = 4

        timeout_val = 0
        continued_count = 0
        while True:
            signal.alarm(600)
            try:
                start = time.time()
                search(queen_count)
                end = time.time()
                continued_count = 0
                if mode == 's':
                    print("{} queens take {} to run".format(
                        queen_count, end-start))
                else:
                    print("({},{})".format(queen_count, end-start), end="")
            except:
                print("\nTime out on {} queens".format(queen_count))
                # break
                continued_count += 1
                if continued_count == 5:
                    break
            queen_count += 1
    
    print("Standard Depth First Search")
    test_timeout(standard_search)
    print('---------------------------')
    print("Back Track Search")
    test_timeout(back_track_search)
    print('---------------------------')
    print("Back track with forward checking")
    test_timeout(bt_fc)
    print('---------------------------')
    print("Back track w/ forward checking and dynamic variable ordering with most constraint variable and least constraining value")
    test_timeout(dvo)
    print('---------------------------')
    print("Back track w/ forward checking and dynamic variable ordering with least constraint variable and most constraining value")
    test_timeout_dvo_lc(dvo)
    print('---------------------------')

if __name__ == "__main__":
    if len(sys.argv) != 1:
        mode = 'l'
    else:
        mode = 's'
    main(mode)

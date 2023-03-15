import sys
import itertools

def diff_summary(postfixA : str, postfixB : str):
    total = 0
    mag = 0
    num = 0
    with open(f"RMSE{postfixA}.txt") as textfile1, open(f"RMSE{postfixB}.txt") as textfile2:
        for x, y in itertools.zip_longest(textfile1, textfile2):
            x = float(x.strip())
            y = float(y.strip())
            total += abs(x-y)
            mag += x-y
            num += 1
        print(f"{postfixA}-{postfixB} diff average = {total/num:.6f}, diff total = {total:.6f}, mag normalized = {mag/total:.6f}, mag = {mag:.6f}")

if __name__ == "__main__":
    diff_summary("_orig", "")
    diff_summary("T_orig", "T")
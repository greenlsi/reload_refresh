import sys
import matplotlib.pyplot as plt
import csv


def main():
    f = []
    for arg in sys.argv[1:]:
        f.append(arg)
        print(arg)
    x = []
    y = []
    x1 = []
    y1 = []
    y2 = []

    with open(f[0], 'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=' ')
        for row in plots:
            x.append(int(row[1]))
            y.append(int(row[0]))
    with open(f[1], 'r') as csvfile:
        plots = csv.reader(csvfile, delimiter=' ')
        for row in plots:
            x1.append(int(row[2]))
            y1.append(int(row[0]))
            y2.append(int(row[1]))
    plt.plot(x, y, 'ro',x1, y1, 'o',x1, y2, 'go')
    plt.ylabel('Time measured (cycles)')
    plt.show()


if __name__ == "__main__":
    main()

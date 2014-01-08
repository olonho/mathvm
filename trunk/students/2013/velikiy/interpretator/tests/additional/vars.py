
import sys

num = int(sys.argv[1])

for i in range(0, num):
    print("int x" + str(i) + ";")

print("x0 = 1;")

for i in range(1, num):
    print("x" + str(i) + " = x" + str(i - 1) + " + " + str(i+1) + ";")

for i in range(0, num):
    print("print(x" + str(i) + ", '\\n');")    


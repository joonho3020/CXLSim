import random

def main():
  for i in range(1, 10000, 1):
    print(i, random.randrange(0, 2))


if __name__=="__main__":
  main()

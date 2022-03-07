import random

def main():
  for i in range(1, 100000000, 1):
    print(random.randrange(1, 1000000), random.randrange(0, 2))


if __name__=="__main__":
  main()

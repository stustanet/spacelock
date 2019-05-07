import random
import subprocess

def randbytes(count):
    return bytes(random.randint(0, 255) for _ in range(count))

def main():
    for i in range(1024):
        data = randbytes(i)
        a = subprocess.check_output(['./sha256test'], input=data)[:-1]
        b = subprocess.check_output(['sha256sum'], input=data)[:-4]

        if a != b:
            print((i, i % 64))
            #print(data)
            #print(a)
            #print(b)

if __name__ == '__main__':
    main()

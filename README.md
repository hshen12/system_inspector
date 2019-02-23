# Project 1: System Inspector

See: https://www.cs.usfca.edu/~mmalensek/cs326/assignments/project-1.html 

To compile and run:

```bash
make
./inspector
```

## Testing

To execute the test cases, use `make test`. To pull in updated test cases, run `make testupdate`. You can also run a specific test case instead of all of them:

```
# Run all test cases:
make test

# Run a specific test case:
make test run=4

# Run a few specific test cases (4, 8, and 12 in this case):
make test run='4 8 12'
```

the code passed all the test except memory leak. our guess is the getpwuid caused. 

the project is reading the file from given directory and tokenizer the string. 
usage:  -a              display all
        -h              help
        -l              task list
        -p procfs_dir   change default directory
        -r              hardware info
        -s              system info
        -t              task info

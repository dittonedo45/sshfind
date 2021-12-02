# sshfind

sshfind is a program you can use to iterate over a ssh host file system.
It is a programmatic alternative of:
```bash
ssh user@host 'find '$path
#Or Alternative using sshfind:
sshfind -u user -k password -p / -t tree
```

#Compile 
Run:
```
cmake . && make
```

# rvcc

如果是通过ssh是用我配置的环境的话，那下面英文的部分就不用看了，我以vscode为例讲一下该怎么玩就可以了，如果想要直接跑的话`make test`应该就可以，如果想要调试的话，安装一个CodeLLDB的插件，参考的launch.json的配置如下

在我本机的编译环境上是用clangd + codelldb作为c/c++的代码提示了，好像这样的搭配比官方的智能提示啥的都要强大一些，但是这个远程机是centos7的版本，实际上很多的东西都不好装，包括那个clangd，所以我就摆烂了。你们看的时候代码的提示效果可能就差了一点。

clangd-style 用的是google的默认风格
```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "type": "lldb",
            "request": "launch",
            "name": "Debug",
            "program": "${workspaceFolder}/rvcc", // 设置
            "args": ["12+34+56"],
            "cwd": "${workspaceFolder}"
        }
    ]
}
```

The installation process is a little more complicated, Linux is required to run(macos probably won't work), so docker is recommended.

Just run the following command, and you're good to go!
```sh
# Note: Please replace "path/to/rvcc" with the actual rvcc path on your computer.
docker run -u root --volume path/to/rvcc:/rvcc --rm -it registry.cn-hangzhou.aliyuncs.com/dailycoding/rvcc
```

After running this, you will enter a terminal environment.

You can simply run

cd /rvcc
make
to build RVCC.

# rvcc

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

riscv64-unkown-linux-gnu-gcc, qemu-riscv64 are often used.
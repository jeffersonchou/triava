1. 解压工具链到opt目录
   tar jxvf android-toolchain.tar.bz2 -C /opt

2. 配置PATH环境变量:
   vi ~/.bashrc
   export PATH=/opt/android-toolchain/bin:${PATH}

3. 执行compile.sh

4. 将out目录下的文件拷贝到电视/data/Triava目录下，并在/data/Triava目录下执行TriavaLoad.sh

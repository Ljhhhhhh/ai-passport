<p align="right">
  <strong>简体中文</strong> · <a href="build-and-test.md">English</a>
</p>

# 构建与验证（Build & Test）

使用 ESP-IDF 5.5.x（已知开发环境 5.5.3）：

```bash
get_idf553                                     # 进入仓库的 ESP-IDF 5.5.3 环境
idf.py -C projects/animal-hanzi-story build     # 编译动物汉字故事固件
idf.py -C projects/hanzi-cards build            # 编译汉字卡片固件
idf.py -C projects/bsp-demo build               # 编译 BSP 硬件演示固件
idf.py -C projects/animal-hanzi-story flash monitor  # 烧录并打开日志
```

仓库提交 `dependencies.lock` 以固定 ESP-IDF Managed Components 的解析结果。修改 `idf_component.yml` 后必须使用 ESP-IDF 5.5.3 重新生成锁文件、review 版本变化并与 manifest 一起提交；普通构建不应产生未提交的锁文件差异。

固件门禁对 `projects/` 下的项目进行编译测试，使用各项目独立的 `sdkconfig.defaults` 在临时目录生成隔离的 `sdkconfig`。它会校验合并后的完整镜像，并将验证后的二进制输出到 `build/<project_name>-full.bin`（及 `build/FoloToy-AI-Passport-full.bin`）。

当前基线包含不依赖硬件的逻辑测试。静态门禁会使用
`-Wall -Wextra -Werror` 编译并运行像素布局、动物识字故事/存档、汉字卡片状态/资源以及
IMA ADPCM/播放器测试。

```bash
./tools/validate.sh --static
```

```bash
./tools/validate.sh --static                      # 仓库一致性、workflow、文档链接、host tests
./tools/validate.sh --firmware animal-hanzi-story # 编译并验证指定项目固件
./tools/validate.sh --firmware                    # 编译并验证 projects/ 下的所有项目
./tools/validate.sh                               # 完整验证（需已激活 ESP-IDF 环境）
```

完整验证要求预先激活 ESP-IDF 5.5.3。CI 与本地使用同一脚本；若 CI 和本地行为不同，应先修复脚本或环境，而不是维护两份命令。

涉及物理外设的改动必须在真机运行硬件指南验收清单，并把“编译通过”与“硬件验证通过”分开记录。

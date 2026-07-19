# openvela 移植到 BK7258 芯片

## 简介

本项目目标是将 openvela RTOS 移植到 Beken BK7258 芯片平台。BK7258 是一款集成 Wi-Fi 6、蓝牙 5.2 的多核 ARM AIoT SoC。本移植工作涵盖 BSP 初始化、基础外设驱动，以及最小 NSH 基线验证。

## 选题方向

新硬件适配

## 目录结构

```
contest2026_223_hepingdechengshi/
├── board/bk7258/   — BK7258 板级支持包（BSP）
├── docs/           — 移植笔记与调试记录
├── logs/           — AI Coding 日志
└── README.md
```

## 编译与运行

### 拉取工程

```bash
repo init -u https://github.com/open-vela/contest2026_223_hepingdechengshi \
  -b dev-ai-contest-2026 -m contest2026_223_hepingdechengshi.xml
repo sync -c -j8
```

### 编译

```bash
./build.sh vendor/beken/boards/contest2026_223_board/configs/nsh/ --cmake -j8 
```


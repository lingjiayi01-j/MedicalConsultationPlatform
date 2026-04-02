# MedicalConsultationPlatform
A medical consultation platform based on Qt 6 C++.
医疗咨询平台（医生端）
基于 Qt 6 C++ 开发的医生端在线问诊系统，为医护人员提供一站式远程医疗服务能力，支持实时问诊、电子处方、排班管理、收益统计等全流程功能，是医疗数字化场景的实用桌面端解决方案。

核心功能
安全登录系统
支持工号 / 手机号 + 密码双模式登录，适配医院内部账号体系
「记住密码」功能，采用加密存储（QSettings / 系统凭据管理器），保障账号安全
登录状态持久化（Token 缓存），支持自动重登与异常重试
登录错误实时提示，输入校验与状态反馈清晰
实时在线问诊
基于 WebSocket 长连接的实时聊天，支持文字、图片、文件多类型消息
患者列表按状态分组（待接诊 / 进行中 / 已完成），未读消息角标提醒
消息气泡式渲染，区分医患消息，自动滚动到底部，历史消息异步加载
网络断线自动重连，消息本地缓存，恢复后自动补发，不丢消息
系统托盘闪烁 + 声音提醒，新消息实时通知，不遗漏问诊
电子处方管理
可视化处方编辑界面，药品信息（名称、剂型、规格、用法用量）全可配置
药品列表增删改查，支持常用药品快捷添加，校验必填项
医生手写签名 / 密码确认，处方合规性保障
一键发送处方给患者，同步归档，支持历史处方追溯
处方数据结构化存储，对接医院 HIS 系统接口
智能排班管理
周视图排班表格，支持周一至周日、早中晚时段自定义
出诊时段启用 / 禁用一键切换，时间范围合规校验
排班数据云端同步，实时更新在线状态，患者端可查看医生出诊时间
排班冲突自动提醒，避免出诊时间重叠
收益统计与提现
今日 / 本月 / 累计收入实时汇总，金额一目了然
Qt Charts 绘制收入趋势折线图，支持按日 / 周 / 月多维度查看
在线提现申请，提现记录列表展示，状态实时更新
收益数据明细可查，对账便捷
个性化设置
个人信息维护（科室、职称、简介），头像上传与预览
密码修改与安全校验，符合医院信息安全规范
通知设置（声音提醒、新消息弹窗），自定义使用习惯
系统托盘、全局快捷键、深色 / 浅色主题切换，适配不同使用场景
以上是目前所有的功能，都是windows客户端方面，后端还在代码审查然后逐步上传
技术栈
开发语言	C++ 17
UI 框架	Qt 6 (Widgets)
网络通信	QNetworkAccessManager (HTTP)、QWebSocket (长连接)
本地存储	QSettings (配置)、SQLite (本地缓存)
数据可视化	Qt Charts
构建工具	qmake
版本控制	Git + GitHub
项目结构
MedicalConsultationPlatform/
├── main.cpp                 # 程序入口，全局初始化
├── logindialog.h/cpp/ui     # 登录窗口
├── mainwindow.h/cpp/ui      # 主窗口，核心界面容器
├── chatwidget.h/cpp/ui      # 聊天界面
├── prescriptiondialog.h/cpp/ui # 处方编辑对话框
├── scheduledialog.h/cpp/ui  # 排班设置对话框
├── settingsdialog.h/cpp/ui  # 个人设置对话框
├── earningsdialog.h/cpp/ui  # 收益统计对话框
├── httpclient.h/cpp         # HTTP 客户端封装（统一接口、异步请求）
├── websocketclient.h/cpp    # WebSocket 长连接管理（自动重连、消息队列）
├── datacache.h/cpp          # 本地数据缓存（SQLite）
├── logger.h/cpp             # 全局日志管理
├── MedicalConsultationPlatform.pro # Qt 项目工程文件
├── README.md                # 项目说明文档
└── .gitignore               # Git 忽略文件配置

环境要求
Qt 6.5 及以上版本（需安装 Charts、Network、Sql 模块）
编译器：MSVC 2022 64-bit / MinGW 64-bit
Windows 10/11 操作系统

开源协议
本项目基于 MIT License 开源，详细协议请查看 LICENSE 文件。

#pragma once

class installer_options {
    // 创建快捷方式选项
    // 安装目录
};

class installer_context {
   private:
    /// @brief 设置当前系统环境变量
    void set_evn();

   public:
    /// @brief 安装应用程序
    void setup();

    /// @brief 更新应用程序
    void update();
};
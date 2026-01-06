// // application.cpp
// #include "application.hpp"

// #include <algorithm>

// #include "window.hpp"


// // 静态成员初始化
// std::unordered_map<HWND, window*> application::s_window_map;
// std::mutex application::s_window_map_mutex;
// bool application::s_window_class_registered = false;

// // 窗口类名
// static const wchar_t* WINDOW_CLASS_NAME = L"window_class";

// application& application::get_instance() {
//     static application instance;
//     return instance;
// }

// const wchar_t* application::get_window_class_name() {
//     return WINDOW_CLASS_NAME;
// }

// void application::register_window_class() {
//     if (s_window_class_registered) {
//         return;
//     }

//     WNDCLASSEXW wc = {};
//     wc.cbSize = sizeof(WNDCLASSEXW);
//     wc.style = CS_HREDRAW | CS_VREDRAW;
//     wc.lpfnWndProc = &application::global_window_proc;
//     wc.cbClsExtra = 0;
//     wc.cbWndExtra = 0;
//     wc.hInstance = GetModuleHandle(nullptr);
//     wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
//     wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
//     wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
//     wc.lpszMenuName = nullptr;
//     wc.lpszClassName = WINDOW_CLASS_NAME;
//     wc.hIconSm = LoadIcon(nullptr, IDI_APPLICATION);

//     s_window_class_registered = RegisterClassExW(&wc) != 0;
//     if (!s_window_class_registered) {
//         throw std::runtime_error("Failed to register window class");
//     }
// }

// LRESULT CALLBACK application::global_window_proc(HWND hwnd, UINT msg,
//                                                  WPARAM w_param,
//                                                  LPARAM l_param) {
//     // 查找窗口实例
//     window* win = find_window(hwnd);

//     if (win) {
//         // 窗口已注册，派遣消息
//         return win->handle_message(msg, w_param, l_param);
//     } else if (msg == WM_NCCREATE) {
//         // 窗口创建时，保存窗口实例
//         LPCREATESTRUCTW cs = reinterpret_cast<LPCREATESTRUCTW>(l_param);
//         window* new_win = reinterpret_cast<window*>(cs->lpCreateParams);

//         if (new_win) {
//             // 在 WM_CREATE 之前注册窗口
//             SetWindowLongPtrW(hwnd, GWLP_USERDATA,
//                               reinterpret_cast<LONG_PTR>(new_win));
//             register_window(hwnd, new_win);

//             // 设置窗口句柄
//             new_win->set_handle_internal(hwnd);

//             // 调用窗口的创建后处理
//             new_win->on_create();

//             // 让窗口处理 WM_NCCREATE
//             return new_win->handle_message(msg, w_param, l_param);
//         }
//     }

//     // 默认处理
//     return DefWindowProcW(hwnd, msg, w_param, l_param);
// }

// LRESULT application::dispatch_to_window(HWND hwnd, UINT msg, WPARAM w_param,
//                                         LPARAM l_param) {
//     window* win = find_window(hwnd);
//     if (win) {
//         return win->handle_message(msg, w_param, l_param);
//     }
//     return DefWindowProcW(hwnd, msg, w_param, l_param);
// }

// void application::register_window(HWND hwnd, window* win) {
//     std::lock_guard<std::mutex> lock(s_window_map_mutex);
//     s_window_map[hwnd] = win;
// }

// void application::unregister_window(HWND hwnd) {
//     std::lock_guard<std::mutex> lock(s_window_map_mutex);
//     s_window_map.erase(hwnd);
// }

// window* application::find_window(HWND hwnd) {
//     std::lock_guard<std::mutex> lock(s_window_map_mutex);
//     auto it = s_window_map.find(hwnd);
//     if (it != s_window_map.end()) {
//         return it->second;
//     }

//     // 尝试从 USERDATA 获取
//     LONG_PTR user_data = GetWindowLongPtrW(hwnd, GWLP_USERDATA);
//     if (user_data) {
//         return reinterpret_cast<window*>(user_data);
//     }

//     return nullptr;
// }

// int application::run() {
//     if (m_running) {
//         throw std::runtime_error("Application is already running");
//     }

//     m_running = true;
//     m_exit_code = 0;

//     // 注册窗口类
//     register_window_class();

//     // 创建所有窗口
//     for (auto& win : m_windows) {
//         if (win && !win->create()) {
//             quit(-1);
//             return m_exit_code;
//         }
//     }

//     // 显示所有窗口
//     for (auto& win : m_windows) {
//         if (win) {
//             win->show();
//         }
//     }

//     // 主消息循环
//     MSG msg = {};

//     while (m_running) {
//         // 处理消息
//         if (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
//             if (msg.message == WM_QUIT) {
//                 m_running = false;
//                 m_exit_code = static_cast<int>(msg.wParam);
//                 break;
//             }

//             // 翻译和分发消息
//             TranslateMessage(&msg);

//             if (msg.hwnd) {
//                 // 有窗口句柄，派遣到对应窗口
//                 DispatchMessageW(&msg);
//             } else {
//                 // 无窗口句柄的消息
//                 DispatchMessageW(&msg);
//             }
//         } else {
//             // 空闲处理
//             for (const auto& callback : m_idle_callbacks) {
//                 if (!m_running) break;
//                 callback();
//             }

//             // 避免CPU占用过高
//             if (m_idle_callbacks.empty()) {
//                 Sleep(1);
//             }
//         }
//     }

//     // 清理
//     std::lock_guard<std::mutex> lock(s_window_map_mutex);
//     s_window_map.clear();

//     return m_exit_code;
// }

// void application::add_window(std::shared_ptr<window> win) {
//     if (!win) {
//         throw std::invalid_argument("Window cannot be null");
//     }

//     m_windows.push_back(win);
// }

// void application::remove_window(HWND hwnd) {
//     m_windows.erase(std::remove_if(m_windows.begin(), m_windows.end(),
//                                    [hwnd](const std::shared_ptr<window>& win) {
//                                        return win && win->get_handle() == hwnd;
//                                    }),
//                     m_windows.end());

//     unregister_window(hwnd);
// }

// void application::quit(int exit_code) {
//     m_exit_code = exit_code;
//     m_running = false;

//     // 销毁所有窗口
//     for (auto& win : m_windows) {
//         if (win && win->get_handle()) {
//             DestroyWindow(win->get_handle());
//         }
//     }
//     m_windows.clear();

//     // 发送退出消息
//     PostQuitMessage(exit_code);
// }
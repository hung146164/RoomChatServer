# RoomChat

Dự án backend server cho ứng dụng chat room sử dụng C++ và CMake. Server này cho phép nhiều client kết nối và trao đổi tin nhắn trong các phòng chat.

## Mục lục

- [Yêu cầu hệ thống](#yêu-cầu-hệ-thống)
- [Cài đặt](#cài-đặt)
- [Xây dựng](#xây-dựng)
- [Chạy](#chạy)
- [Cấu trúc dự án](#cấu-trúc-dự-án)
- [Đóng góp](#đóng-góp)
- [Giấy phép](#giấy-phép)

## Yêu cầu hệ thống

- CMake (phiên bản 3.10 trở lên)
- Trình biên dịch C++ (như g++ hoặc clang++)
- Hệ điều hành Linux (hoặc tương thích với socket programming)

## Cài đặt

1. Clone repository này về máy của bạn:
   ```
   git clone <repository-url>
   cd RoomChat
   ```

## Xây dựng

1. Tạo thư mục build:
   ```
   mkdir build
   cd build
   ```

2. Chạy CMake để tạo Makefile:
   ```
   cmake ..
   ```

3. Biên dịch dự án:
   ```
   make
   ```

Sau khi xây dựng thành công, file thực thi `BackendServer` sẽ được tạo trong thư mục `build/`.

## Chạy

Chạy server từ thư mục build:
```
./BackendServer
```

Server sẽ lắng nghe trên cổng mặc định (có thể cấu hình trong code). Client có thể kết nối để tham gia chat.

## Cấu trúc dự án

- `src/`: Chứa mã nguồn C++
  - `main.cpp`: Điểm vào của chương trình
  - `server.cpp`: Logic của server
- `include/`: Header files
  - `server.h`: Khai báo các hàm và cấu trúc
- `CMakeLists.txt`: File cấu hình CMake
- `build/`: Thư mục build (tạo sau khi chạy CMake)

## Đóng góp

Nếu bạn muốn đóng góp cho dự án, vui lòng tạo một pull request hoặc mở issue trên GitHub.

## Giấy phép

Dự án này sử dụng giấy phép MIT. Xem file LICENSE để biết thêm chi tiết.





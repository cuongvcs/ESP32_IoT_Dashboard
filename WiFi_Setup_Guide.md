# Hướng dẫn Thiết lập WiFi Thông minh (Captive Portal) cho ESP32

Bạn vừa nâng cấp thành công hệ thống lên chuẩn công nghiệp! 
Từ bây giờ, mạch ESP32 của bạn đã hoàn toàn "cai nghiện" việc bị cắm cáp USB để sửa pass WiFi. Thiết bị giờ linh động 100%.

Dưới đây là các bước quy trình tải code và test thử nghiệm.

## BƯỚC 1: Cài đặt thư viện `WiFiManager`
Mã nguồn mới cần thư viện `WiFiManager` để tự động phát sóng WiFi.
1. Mở Arduino IDE.
2. Vào **Tools -> Manage Libraries...** (hoặc nhấn `Ctrl + Shift + I`).
3. Gõ vào ô tìm kiếm: **`wifimanager`**.
4. Tìm đến thư viện có tên chính xác là: **`WiFiManager` của tác giả `tzapu`**.
5. Nhấn **Install** để cài đặt.

## BƯỚC 2: Nạp mã nguồn
1. Bạn mở lại file `ESP32_IoT_Dashboard.ino` trên máy tính.
2. Chọn đúng Port (cổng COM) và Board ESP32 như mọi khi.
3. Nhấn **Upload** (hoặc `Ctrl + U`).
4. Chờ máy IDE báo **Done Uploading**. Tốt nhất là mở **Serial Monitor** (115200 baud) để xem log chạy.

---

## BƯỚC 3: Test thử nghiệm mạng mới (Rất thú vị)
Hãy thử giả lập tình huống bạn mang ESP32 đi một nơi khác hoàn toàn không có WiFi ở nhà:

1. **Khi cắm điện ESP32**: Nó sẽ cố tìm WiFi cũ. Vì nó không mò ra hoặc mò không được (nếu bạn nạp code mới lần đầu nó sẽ trắng bóc thông tin), nó sẽ **chuyển chế độ: Tự phát ra WiFi**.
2. **Lấy điện thoại của bạn ra**: Vào phần dò WiFi trên điện thoại, bạn sẽ thấy một mạng WiFi mới toanh tên là: **`ESP32_Setup`**.
3. **Kết nối vào mạng đó**: 
   - Nhập mật khẩu là: **`12345678`** (đây là pass mặc định tôi gán trong code để bảo mật không cho người lạ đổi pass của bạn).
4. **Màn hình ma thuật hiện ra**: Ngay khi điện thoại bạn kết nối được vào `ESP32_Setup`, hoặc là điện thoại tự đẩy ra cửa sổ đăng nhập, hoặc bạn tự mở trình duyệt web lên và gõ IP: **`192.168.4.1`**.
5. Trình duyệt (có giao diện Dark Theme xịn xò) sẽ hỏi bạn muốn ESP32 kết nối vào WiFi nội bộ nào. 
   - Nhấn mục **Configure WiFi**.
   - Nó sẽ quét tất cả WiFi hiện có xung quanh. Bạn chọn mạng nhà bạn và nhập Pass của mạng nhà bạn.
   - Nhấn **Save**.
6. **Thành công!**: ESP32 sẽ lập tức tắt cái mạng `ESP32_Setup` đi, tự khởi động lại bộ nhớ và kết nối vào thông tin mảng xịn mà bạn vừa kết nối! Trên Dashboard Web của bạn sẽ sáng đèn `ESP: Connected` luôn!

> **Lưu ý:** Lần tới bạn khởi động lại, nó sẽ tự động vào mạng cũ. Nếu bạn đem đi tỉnh khác, quá trình phát ra `ESP32_Setup` sẽ được lặp lại tự động! Cứ thoải mái rút điện và mang đi bất kỳ đâu.

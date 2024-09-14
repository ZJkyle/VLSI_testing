以下是整理後的 README 文件，以 Markdown 語法呈現：

```markdown
# ATPG Tool Usage and Structure

## 使用方式

```bash
$ ./atpg -help
```

**Usage:**
```bash
atpg [options] input_circuit_file
```

### Available Options:
- `-help` : 顯示此幫助摘要
- `-logicsim` : 執行邏輯模擬
- `-plogicsim` : 執行平行邏輯模擬
- `-fsim` : 執行 stuck-at 錯誤模擬
- `-stfsim` : 執行單一模式單一轉換錯誤模擬
- `-transition` : 執行轉換錯誤 ATPG
- `-input <$val>` : 設定輸入模式檔案
- `-output <$val>` : 設定輸出模式檔案
- `-bt [$val]` : 設定回溯限制

---

## Command Line Interface Example

### Logic Simulation:
```bash
./atpg -logicsim -input <pattern> <circuit>
```

### 設定（在 `main.cc` 中的 SetupOption 函數）
```cpp
option.enroll(Name, InputValue, Description, 0);
```

#### InputValue 類型：
- `GetLongOpt::NoValue` : 無需輸入值
- `GetLongOpt::MandatoryValue` : 需要輸入值
- `GetLongOpt::OptionalValue` : 可選輸入值

### 範例：
```cpp
option.enroll("logicsim", GetLongOpt::NoValue, "run logic simulation", 0);
option.enroll("input", GetLongOpt::MandatoryValue, "set the input pattern file", 0);
```

---

## Arguments and Usage

### 檢查是否在命令行中指定某個選項：
```cpp
option.retrieve(name_of_argument)
```

### 範例：
```cpp
// 如果選項 “-logicsim” 被設定
if (option.retrieve("logicsim")) {
    // 執行邏輯模擬器的程式碼
}
```

---

## Pattern Format

- 第一行為 Primary Input (PI) 名稱
- 其他行為 Primary Input 值

### 範例：`s27.bench` 的 7 個測試模式
```text
PI G0 PI G1 PI G2 PI G3 PI G5 PI G6 PI G7
1100011
0110010
1001011
1001010
1101010
0011111
1011100
```

---

## Data Structures – Class GATE

### Assignment #0 使用的資料成員（在 `gate.h` 中）：
- `Name` : gate 名稱
- `Function` : gate 功能，包括：
  - `G_PI`, `G_PO`, `G_PPI`, `G_PPO`
  - `G_NOT`, `G_BUF`, `G_DFF`
  - `G_AND`, `G_NAND`, `G_OR`, `G_NOR`
- `Input_list/Output_list` : fanin/fanout gate 列表

### 後續作業中使用的資料成員：
- `Value`, `Flag`, `Fault status`, `ATPG status`, 等等

---

## Data Structures – GATE 範例

- Gate 的名稱源自其輸出網路
- 範例：`Gate net31`
  - `Name = net31`
  - `Function = G_NAND`
  - `Input_list = {net58, n102, G6}`
  - `Output_list = {net30, G11}`

#### 範例電路：`s27.bench`
```text
net31 = NAND(net58, n102, G6)
```

### 網路計數：
- **主幹網數量**：1
  - `net31`
- **分支網數量**：2
  - `net31 -> net30`
  - `net31 -> G11`
```

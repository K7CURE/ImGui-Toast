# ImGui Toast 

一个为Dear ImGui设计的现代化、高性能Toast通知，具有流畅的动画和物理效果。

### 基本用法

```cpp
#include "Toast.h"

// Ordinary
Custom::Toast::Show("Ok", "123");

// Error
Custom::Toast::Show("no", "oh，no", 5.0f, true);

// Update
Custom::Toast::UpdateAndRender();

﻿# TCP_project

### 簡介

實作一個server可以同時處理多個client上傳下載檔案以及影片播放，並且在bad network( delay 500ms )時仍可以正常傳送檔案和播放影片。影片播放使用C++中的openCV實現，使用fork()處理多個client。

### 如何執行

請在linux的環境執行make指令

```
$ make
```

#### server執行

```
$ ./server [port number]
```

若有client連上將顯示client的id number，當client斷連線也會輸出client已斷線

#### client 執行

```
$ ./client [client_ID] [IP address]:[port number]
```

###### client可以下達的指令

+ 列出server中目前有哪些檔案

  ```
  $ ls
  ```

+ 上傳多個檔案

  ```
  $ put file1 file2 file3
  ```

+ 下載多個檔案

  ```
  $ get file1 file2 file3
  ```

+ 播放影片 ==只能接受mpg file的影片==

  ```
  $ play video.mpg
  ```

  



​	








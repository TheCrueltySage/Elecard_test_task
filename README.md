# Elecard_test_task

Тестовое задание, сделанное на собеседование в компанию Элекард.

Принимает на вход несжатое видео c цветовой палитрой YUV420p, его характеристики (высоту, ширину, количество фреймов) и изображение BMP в формате RGB24 или RGB32.  
Накладывает изображение на видео посередине, сохраняет полученное видео в новый файл, имеет опцию отрендерить полученное изображение с помощью OpenGL в новом окне.

Конверсия и наложение мультитредовы. Конверсия использует алгоритм SSE2 при наличии SSE2 либо при передаче соответствующего параметра CMake.

Содержит тесты CTest, которые сверяют 
* результаты работы одноготредовой и мультитредовой конверсии. 
* результатов работы и времени выполнения скалярного и векторного алгоритмов.
Тесты требуют наличия test.bmp и test.yuv предустановленного размера в папке test_data.

---
## Зависимости:
* GNU getopt/ultragetopt (включён как субмодуль, собирается для платформ без getopt_long)
* GLFW
* GLM

---
## Инструкция по сборке:
Наберите следующие команды в директории build для того, чтобы собрать приложение:
```
cmake ..
make
```

Используйте для тестирования:
```
ctest ..``
```

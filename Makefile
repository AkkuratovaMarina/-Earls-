#   make all        — собрать библиотеку и консольное приложение (по умолчанию)
#   make lib        — собрать только статическую библиотеку libgraphdro4.a
#   make app        — собрать только консольное приложение graphdro4
#   make test       — собрать и запустить юнит-тесты (Catch2)
#   make clean      — удалить все сгенерированные файлы (объектники, библиотеку, исполняемые)
#   make install    — установить библиотеку и заголовки в системные пути (обычно требует sudo)
#   make docs       — сгенерировать документацию через Doxygen (если есть Doxyfile)

#   CXX        — компилятор C++ (по умолчанию g++; можно задать clang++, g++-12, ...)
#   CXXFLAGS   — флаги компиляции (по умолчанию -std=c++17 -Wall -Wextra -O2)
#   LDFLAGS    — дополнительные флаги линковки (по умолчанию пусто)
#   AR         — архиватор для статической библиотеки (по умолчанию ar)
#   ARFLAGS    — флаги архиватора (по умолчанию rcs)

#   Собрать всё с GCC (стандартный вариант)- make all
#   Собрать всё с указанием конкретного компилятора и с оптимизацией по размеру - make CXX=clang++ CXXFLAGS="-std=c++17 -Os -Wall"
#   Собрать только тесты и сразу их запустить (результат в консоли) - make test
#   Удалить всё и пересобрать с нуля - make clean && make all

# Компилятор и флаги
CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra
LDFLAGS  ?=

SRCDIR   := .
# Директории
OBJDIR   := build
# Все .cpp файлы в корне
SOURCES  := $(wildcard *.cpp)
# Объектные файлы в build/
OBJECTS  := $(addprefix $(OBJDIR)/, $(notdir $(SOURCES:.cpp=.o)))
# Исключаем main.cpp и tests_graphdro4.cpp из библиотеки
LIB_SOURCES   := $(filter-out main.cpp tests_graphdro4.cpp, $(SOURCES))
LIBOBJS       := $(addprefix $(OBJDIR)/, $(notdir $(LIB_SOURCES:.cpp=.o)))

# Имена исполняемых файлов
ifeq ($(OS),Windows_NT)
TEST_BIN := tests_graphdro4.exe
MAIN_BIN := graphdro4.exe
else
TEST_BIN := ./tests_graphdro4
MAIN_BIN := ./graphdro4
endif

.PHONY: all clean test run-help
# Создание директории build/
$(OBJDIR):
	mkdir -p $@

# Правило компиляции: из %.cpp -> build/%.o
$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Сборка основного приложения
graphdro4: $(OBJDIR)/main.o $(LIBOBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# Сборка тестов
tests_graphdro4: $(LIBOBJS) tests_graphdro4.cpp
	$(CXX) $(CXXFLAGS) -o $@ $(LIBOBJS) tests_graphdro4.cpp $(LDFLAGS)

# Цель по умолчанию
all: graphdro4 tests_graphdro4

# Запуск тестов
test: tests_graphdro4
	"$(TEST_BIN)"

# Запуск help
run-help: graphdro4
	"$(MAIN_BIN)" help

# Очистка
clean:
ifeq ($(OS),Windows_NT)
	-if exist build rmdir /s /q build
	-del /q graphdro4.exe tests_graphdro4.exe graphdro4 tests_graphdro4 2>nul
else
	rm -rf $(OBJDIR) graphdro4 tests_graphdro4
endif

UNAME := $(shell uname)

ifeq ($(UNAME),Linux)
	INCLUDES = $(LINUX_INCLUDES)
	LINKS = $(LINUX_LINKS)
else ifeq ($(UNAME),Darwin)
	INCLUDES = $(MAC_INCLUDES)
	LINKS = $(MAC_LINKS)
else
	$(error Unsupported OS: $(UNAME))
endif

CPPC = c++

CFLAG = -Wall -Wextra -Werror -std=c++20 -O3 -DNDEBUG

SFML_VERSION = 3.1.0
SFML_ARCHIVE = SFML-$(SFML_VERSION)-linux-gcc-64-bit.tar.gz
SFML_URL = https://www.sfml-dev.org/files/$(SFML_ARCHIVE)
SFML_DIR = SFML-$(SFML_VERSION)

MAC_INCLUDES = -Iincludes \
		-I$$(brew --prefix sfml)/include \

MAC_LINKS = -L$$(brew --prefix sfml)/lib \
	-lsfml-graphics -lsfml-window -lsfml-system

LINUX_INCLUDES = -Iincludes \

LINUX_LINKS = -I"${HOME}/.local/include" \
		-L"${HOME}/.local/lib" \
		-Wl,-rpath,"${HOME}/.local/lib" \
		-lsfml-graphics -lsfml-window -lsfml-system

SRCS = src/Gomoku.cpp \
	   src/Rules.cpp \
	   src/Solver.cpp \
	   src/UI.cpp \
	   src/main.cpp

OBJS = $(SRCS:.cpp=.o)

NAME = Gomoku

all: $(NAME)

$(NAME): $(OBJS)
	$(CPPC) $(CFLAG) $(OBJS) $(LINKS) -o $(NAME)

%.o:%.cpp $(wildcard includes/*.hpp)
	$(CPPC) $(INCLUDES) $(CFLAG) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all

install-fedora:
	sudo dnf install -y SFML-devel

install-debian:
	sudo apt-get install -y libsfml-dev

install-mac:
	brew install sfml -y

# Works at42 without sudo
install-linux:
	@echo "Installing SFML $(SFML_VERSION)..."
	wget -q $(SFML_URL) -O /tmp/$(SFML_ARCHIVE)
	tar -xzf /tmp/$(SFML_ARCHIVE) -C /tmp
	mkdir -p "$$HOME/.local/include" "$$HOME/.local/lib"
	cp -r /tmp/$(SFML_DIR)/include/SFML "$$HOME/.local/include/"
	cp -r /tmp/$(SFML_DIR)/include/SFML "./includes/"
	cp -r /tmp/$(SFML_DIR)/lib/* "$$HOME/.local/lib/"
	rm -rf /tmp/$(SFML_ARCHIVE) /tmp/$(SFML_DIR)
	@echo "SFML installed in $$HOME/.local"

.PHONY: all clean fclean re

# Headless tactical suite; SFML libraries and a display are not required.
test:
	$(CPPC) -std=c++20 -O2 -Wall -Wextra -Werror -ffunction-sections -fdata-sections -Iincludes tests/solver_tests.cpp src/Solver.cpp src/Rules.cpp -o /tmp/gomoku-tests
	/tmp/gomoku-tests

.PHONY: test

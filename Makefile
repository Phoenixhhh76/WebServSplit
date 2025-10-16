# **************************************************************************** #
#                                   SETTINGS                                   #
# **************************************************************************** #

NAME        := webserv
CXX         := c++
CXXFLAGS    := -Wall -Wextra -Werror -std=c++98
DEBUGFLAGS  := -g -DDEBUG
RM          := rm -rf

SRC_DIR     := src
BUILD_DIR   := build
MAIN_SRC    := main.cpp
MAIN_OBJ    := $(BUILD_DIR)/main.o

# search recursively for all cpp files in src/
SRC_FILES   := $(shell find $(SRC_DIR) -type f -name '*.cpp')
OBJ_FILES   := $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_FILES))

INC_FLAGS   := -I$(SRC_DIR) \
               -I$(SRC_DIR)/app \
               -I$(SRC_DIR)/config \
               -I$(SRC_DIR)/core \
               -I$(SRC_DIR)/net \
               -I$(SRC_DIR)/http

# **************************************************************************** #
#                                   TARGETS                                    #
# **************************************************************************** #

all: $(NAME)

$(NAME): $(OBJ_FILES) $(MAIN_OBJ)
	@echo "🔧 Linking $(NAME)..."
	$(CXX) $(CXXFLAGS) $(OBJ_FILES) $(MAIN_OBJ) -o $(NAME)
	@echo "✅ Build complete!"

# compile src/**/*.cpp → build/**/*.o
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $< -o $@

# compile main.cpp → build/main.o  (needs INC_FLAGS!)
$(MAIN_OBJ): $(MAIN_SRC)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(INC_FLAGS) -c $(MAIN_SRC) -o $(MAIN_OBJ)

# **************************************************************************** #
#                                  UTILITIES                                   #
# **************************************************************************** #

clean:
	@echo "🧹 Cleaning object files..."
	$(RM) $(BUILD_DIR)

fclean: clean
	@echo "🧹 Removing binary..."
	$(RM) $(NAME)

re: fclean all

debug:
	$(MAKE) CXXFLAGS="$(CXXFLAGS) $(DEBUGFLAGS)" re

.PHONY: all clean fclean re debug

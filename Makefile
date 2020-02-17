LOGIN		= `whoami`

# **************************************************************************** #
#                                   BINARIES                                   #
# **************************************************************************** #

NAME		= aiced

# **************************************************************************** #
#                                  COMPILATION                                 #
# **************************************************************************** #

CC 			= g++ -std=c++17
CFLAGS		= -Wall -Wextra #-Werror

FSAN		= #-fsanitize=address
DEBUG		= #-g3
OPTI		= #-Ofast

# **************************************************************************** #
#                                 DIRECTORIES                                  #
# **************************************************************************** #

SRCDIR		= srcs
OBJDIR		= .objs
INC			= -I ../opencv_dir/include/
LIB			= -L ../opencv_dir/lib/ -lopencv_highgui.4.2.0 -lopencv_imgproc.4.2.0 -lopencv_core.4.2.0 -lopencv_imgcodecs.4.2.0 -lopencv_video.4.2.0 -lopencv_videoio.4.2.0 -lopencv_calib3d.4.2.0 -lopencv_highgui.4.2

# **************************************************************************** #
#                                 INCLUDES                                     #
# **************************************************************************** #


# **************************************************************************** #
#                                  SOURCES                                     #
# **************************************************************************** #

SRCS 		=		main.cpp

# **************************************************************************** #
#                                   UTILS                                      #
# **************************************************************************** #

OBJS		= $(addprefix $(OBJDIR)/, $(SRCS:.c=.o))

DPDCS		= $(OBJS:.o=.d)

# **************************************************************************** #
#                                   RULES                                      #
# **************************************************************************** #

all				:
	@make -j 4 $(NAME)

$(NAME)			:
	$(CC) $(CFLAGS) $(INC) $(LIB) $(SRCS)

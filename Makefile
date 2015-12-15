DIR_KERNEL=./kernel
DIR_IO=./io
DIR_SV=./servo


.PHONY : all kernel io sv clean rebuild

all: kernel io sv

kernel:
	@make -C $(DIR_KERNEL)

io:
	@make -C $(DIR_IO)

sv:
	@make -C $(DIR_SV)

clean:
	@make clean -C $(DIR_KERNEL)
	@make clean -C $(DIR_IO)
	@make clean -C $(DIR_SV)

rebuild: clean all

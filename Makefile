CUR_DIR=$(shell pwd)

HTTP_DIR=${CUR_DIR}/lucashttp
THREAD_DIR=${CUR_DIR}/lucasthread
MAIN_DIR=${CUR_DIR}

SRC1 = ${wildcard  ${HTTP_DIR}/*.cpp} \
      ${wildcard  ${THREAD_DIR}/*.cpp}
OBJA = ${patsubst %.cpp, %.a, ${SRC1}}
SRC2 = ${wildcard  ${HTTP_DIR}/*.cpp} \
      ${wildcard  ${THREAD_DIR}/*.cpp} \
      ${wildcard  ${MAIN_DIR}/*.cpp}
OBJO = ${patsubst %.cpp, %.o, ${SRC2}}
OBJT = $(OBJO:d.o=d.a)
OBJF = $(OBJT:p.o=p.a)
TARGET=main
CC=g++

# test:
# 	@echo $(SRC1)
# 	@echo $(SRC2)
# 	@echo $(OBJO)
# 	@echo $(OBJA)
# 	@echo $(OBJF)


${TARGET}: ${OBJF}
	${CC} -o $@ ${OBJO} -lpthread
	@echo "Compile done."

$(OBJA):%.a:%.o
	@echo "Linking $< ==> $@"
	ar -rc $@ $<

$(OBJO):%.o:%.cpp
	@echo "Compiling $< ==> $@"
	${CC} -c $< -o $@ -lpthread

clean:
	@rm -f ${OBJO}
	@echo "Clean object files done."

	@rm -f ${OBJA}
	@echo "Clean link files done."

	@rm ${TARGET}
	@echo "Clean target files done."

	@echo "Clean done."

docker build -t compilator .
docker run --rm -v ${PWD}/oCodeExamples:/data compilator /data/myExample.o

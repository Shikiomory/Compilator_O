#Сборка контейнера
docker build -t compilator .
#Запуск контейнера
docker run --rm -v ${PWD}/oCodeExamples:/data compilator /data/myExample.o

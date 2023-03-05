FROM sehrig/cling:0.7-dev

# Копируем скрипт в контейнер
COPY /bin/everest.sh /root/everest.sh

COPY jupyter_notebook_config.py /root/.jupyter/

# Устанавливаем рабочую директорию
WORKDIR /root/cgen

RUN mkdir -p /root/bin

# Копируем исходные файлы С++ в контейнер
COPY /cgen/skel.cpp /cgen/cgen.cpp /cgen/lexer.cpp /cgen/parse.cpp /cgen/acta.cpp /root/cgen/

# Компилируем файлы
RUN cd /root/cgen && \
    g++ skel.cpp -o skel && \
    g++ cgen.cpp lexer.cpp parse.cpp -o cgen && \
    g++ acta.cpp -o acta && \
    mv /root/cgen/skel /root/bin/skel && \
    mv /root/cgen/cgen /root/bin/cgen && \
    mv /root/cgen/acta /root/bin/acta && \
    chmod 755 /root/bin/skel && \
    chmod 755 /root/bin/cgen && \
    chmod 755 /root/bin/acta && \
    chmod 755 /root/everest.sh

# Устанавливаем переменную окружения PATH
ENV PATH="/root/bin:${PATH}"

# Устанавливаем рабочую директорию по умолчанию
WORKDIR /root
FROM gcc 
WORKDIR /app/
COPY . /app/ 

RUN ["/bin/bash"]
RUN ["make"]

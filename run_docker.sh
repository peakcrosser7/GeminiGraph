#!/bin/bash

image_name="gemini_graph_img:0.0.1"
container_name="hhy_develop_gemini"

# YOU NEED TO SPECIFY!!!
declare -a volumes=(
    # 
)

volume_args=""
for vol in "${volumes[@]}"; do
    volume_args+=" -v ${vol}"
done

echo ">>> Building docker image: ${image_name}"
docker build -t ${image_name} .

if [ $? -ne 0 ]; then
    echo ">>> Error: Docker build failed"
    exit 1
fi

if docker ps -a | grep -q ${container_name}; then
    echo ">>> Error: Container ${container_name} already exists"
    exit 2
fi

echo ">>> Starting container: ${container_name}"
docker run -it \
    --privileged \
    --name ${container_name} \
    --hostname ${container_name} \
    ${volume_args} \
    ${image_name}

# Build stage
FROM debian:trixie-slim AS build

# Install necessary build tools and dependencies
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    make \
    git \
    wget \
    libcurl4-openssl-dev \
    zlib1g-dev \
    ca-certificates \
    libssl-dev \
    libopus0 \
    libopus-dev \
    libsodium-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory
WORKDIR /app

# Copy the source code
COPY . .

# Create build directory
RUN mkdir build

# Build the project
WORKDIR /app/build
RUN wget -O libdpp.deb https://github.com/brainboxdotcc/DPP/releases/download/v10.0.23/libdpp-10.0.23-linux-x64.deb \
    && dpkg -i libdpp.deb \
    && ldconfig
RUN cmake -DUSE_EXTERNAL_JSON=OFF .. -B./ && make

# Run stage
FROM debian:trixie-slim AS runtime

# Install runtime dependencies & libraries
RUN apt-get update && apt-get install -y \
    ca-certificates \
    libopus0 \
    libopus-dev \
    libsodium-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

# Create a non-root user
RUN useradd -m -s /bin/bash appuser

# Set the working directory
WORKDIR /app

# Add external library libdpp
COPY --from=build /app/build/libdpp.deb .
RUN dpkg -i libdpp.deb && rm libdpp.deb

# Copy the built executable from the build stage
COPY --from=build --chown=appuser:appuser /app/build/asiantbd_bot .

# Switch to non-root user
USER appuser

# Set the entrypoint
ENTRYPOINT ["/app/asiantbd_bot"]

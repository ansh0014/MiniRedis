# Build stage
FROM node:18-alpine AS builder

WORKDIR /app

# Copy package files
COPY package.json package-lock.json* ./
RUN npm ci

# Copy source code
COPY . .

# Expose Vite dev server port
EXPOSE 5173
ENV VITE_HOST=0.0.0.0

# Run development server
CMD ["npm", "run", "dev"]
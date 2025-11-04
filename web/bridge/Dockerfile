FROM node:20-alpine
WORKDIR /app
COPY package.json package-lock.json* ./
RUN npm install --production
COPY server.js ./
COPY client ./client
ENV PORT=8081
EXPOSE 8081
CMD ["npm","start"]

FROM nginx:1.27-alpine

RUN rm -rf /usr/share/nginx/html/* \
    && rm /etc/nginx/conf.d/default.conf

COPY nginx.conf /etc/nginx/conf.d/default.conf
COPY gallery /usr/share/nginx/html/gallery
COPY dist /usr/share/nginx/html/dist

EXPOSE 80

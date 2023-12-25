FROM nginx:1.23.4

RUN rm /etc/nginx/conf.d/default.conf
COPY trentos.conf /etc/nginx/conf.d/trentos.conf
COPY index.html /srv/http/www/index.html
COPY self-signed.crt /etc/nginx/
COPY self-signed.key /etc/nginx/
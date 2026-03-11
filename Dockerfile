# Source - https://stackoverflow.com/a/58129973
# Posted by Adiii, modified by community. See post 'Timeline' for change history
# Retrieved 2026-03-11, License - CC BY-SA 4.0

FROM alpine AS build-env
RUN apk add --no-cache build-base
WORKDIR /app
COPY . .
RUN gcc -o server main.c
FROM alpine
COPY --from=build-env /app/server /app/server
WORKDIR /app
CMD ["/app/server"]


CREATE EXTENSION adminpack; /* complete postgresql-contrib installation */
CREATE ROLE :username WITH NOSUPERUSER CREATEDB INHERIT LOGIN ENCRYPTED PASSWORD :password;

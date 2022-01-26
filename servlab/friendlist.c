// Grade received: 100/100

/*
 * friendlist.c - [Starting code for] a web-based friend-graph manager.
 *
 * Based on:
 *  tiny.c - A simple, iterative HTTP/1.0 Web server that uses the 
 *      GET method to serve static and dynamic content.
 *   Tiny Web server
 *   Dave O'Hallaron
 *   Carnegie Mellon University
 * 
 */

#include "csapp.h"
#include "dictionary.h"
#include "more_string.h"

static void doit(int fd);
static dictionary_t *read_requesthdrs(rio_t *rp);
static void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *d);
static void clienterror(int fd, char *cause, char *errnum, 
                        char *shortmsg, char *longmsg);
static void print_stringdictionary(dictionary_t *d);
//static void serve_request(int fd, dictionary_t *query);

static void *handle_request(void *connfd_ptr);
static void redirect_request(int fd, char *body);
static void serve_friends(int fd, dictionary_t *query);
static void serve_introduce(int fd, dictionary_t *query);
static void serve_befriend(int fd, dictionary_t *query);
static void serve_unfriend(int fd, dictionary_t *query);

pthread_mutex_t lock;
dictionary_t *friendlist;


/*                                                                                                 
 * *ok_header -                                                                                    
 */
static char *ok_header(size_t len, const char *content_type) {
  char *len_str, *header;

  header = append_strings("HTTP/1.0 200 OK\r\n",
                          "Server: Friendlist Web Server\r\n",
                          "Connection: close\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n",
                          "Content-type: ", content_type, "\r\n\r\n",
                          NULL);
  free(len_str);

  return header;
}


/*
 * main - main function
 */
int main(int argc, char **argv) {
  int listenfd, connfd;
  char hostname[MAXLINE], port[MAXLINE];
  socklen_t clientlen;
  struct sockaddr_storage clientaddr;
  
  /* Check command line args */
  if (argc != 2) {
    fprintf(stderr, "usage: %s <port>\n", argv[0]);
    exit(1);
  }

  pthread_mutex_init(&lock, NULL);
  listenfd = Open_listenfd(argv[1]);
  friendlist = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);

  /* Don't kill the server if there's an error, because
     we want to survive errors due to a client. But we
     do want to report errors. */
  exit_on_error(0);

  /* Also, don't stop on broken connections: */
  Signal(SIGPIPE, SIG_IGN);

  while (1) {
    clientlen = sizeof(clientaddr);
    connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
    if (connfd >= 0) {
      Getnameinfo((SA *) &clientaddr, clientlen, hostname, MAXLINE, 
                  port, MAXLINE, 0);
      printf("Accepted connection from (%s, %s)\n", hostname, port);
      // doit(connfd);
      // Close(connfd);

      int *connfd_ptr;
      pthread_t conn_thread;
      connfd_ptr = malloc(sizeof(int));
      *connfd_ptr = connfd;
      Pthread_create(&conn_thread, NULL, handle_request, connfd_ptr);
      Pthread_detach(conn_thread);
    }
    else
      {
	printf("Failed to connect");
      }
  }
}


/*
 * doit - handle one HTTP request/response transaction
 */
void doit(int fd) {
  char buf[MAXLINE], *method, *uri, *version;
  rio_t rio;
  dictionary_t *headers, *query;

  /* Read request line and headers */
  Rio_readinitb(&rio, fd);
  if (Rio_readlineb(&rio, buf, MAXLINE) <= 0)
    return;
  printf("%s", buf);
  
  if (!parse_request_line(buf, &method, &uri, &version)) {
    clienterror(fd, method, "400", "Bad Request",
                "Friendlist did not recognize the request");
  } else {
    if (strcasecmp(version, "HTTP/1.0")
        && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
                  "Friendlist does not implement that version");
    } else if (strcasecmp(method, "GET")
               && strcasecmp(method, "POST")) {
      clienterror(fd, method, "501", "Not Implemented",
                  "Friendlist does not implement that method");
    } else {
      headers = read_requesthdrs(&rio);

      /* Parse all query arguments into a dictionary */
      query = make_dictionary(COMPARE_CASE_SENS, free);
      parse_uriquery(uri, query);
      if (!strcasecmp(method, "POST"))
        read_postquery(&rio, headers, query);

      /* For debugging, print the dictionary */
      // print_stringdictionary(query);

      /* You'll want to handle different queries here,
         but the intial implementation always returns
         nothing: */
      // serve_request(fd, query);

      if (starts_with("/friends", uri)) {
	pthread_mutex_lock(&lock);
	serve_friends(fd, query);
	pthread_mutex_unlock(&lock);
      }
      else if (starts_with("/introduce", uri)) {
	serve_introduce(fd, query);
      }
      else if (starts_with("/befriend", uri)) {
	pthread_mutex_lock(&lock);
	serve_befriend(fd, query);
	pthread_mutex_unlock(&lock);
      }
      else if (starts_with("/unfriend", uri)) {
	pthread_mutex_lock(&lock);
	serve_unfriend(fd, query);
	pthread_mutex_unlock(&lock);
      }

      /* Clean up */
      free_dictionary(query);
      free_dictionary(headers);
    }

    /* Clean up status line */
    free(method);
    free(uri);
    free(version);
  }
}


/*                                                                                                
 * *handle_request - request handler                                                              
 */
void *handle_request(void *connfd_ptr) {
  int connfd = *(int *)connfd_ptr;

  free(connfd_ptr);
  doit(connfd);
  Close(connfd);

  return NULL;
}


/*
 * redirect_request - redirects header request
 */
static void redirect_request(int fd, char *body) {
  char *header;
  size_t len = strlen(body);

  header = ok_header(len, "text/html; charset=utf-8");
  Rio_writen(fd, header, strlen(header));
  printf("Response headers:\n");
  printf("%s", header);
  free(header);
  Rio_writen(fd, body, len);
}


/*
 * serve_friends - friend request handler
 */
static void serve_friends(int fd, dictionary_t *query) {
  char *body;
  print_stringdictionary(query);

  if (dictionary_count(query) != 1) {
    clienterror(fd, "GET", "400", "Bad Request",
		"/friends needs at least one user.");
  }

  const char *user = dictionary_get(query, "user");
  
  if (user == NULL) {
    clienterror(fd, "GET", "400", "Bad Request",
		"Bad user input.");
  }

  dictionary_t *user_friends = dictionary_get(friendlist, user);
  
  if (user_friends == NULL) {
    body = "";
    redirect_request(fd, body);
  }
  else {
    const char **all_friends = dictionary_keys(user_friends);

    print_stringdictionary(user_friends);
    body = join_strings(all_friends, '\n');
    printf("The user: %s\n", user);
    printf("The body: %s\n", body);
    redirect_request(fd, body);
  }  
}


/*
 * serve_introduce - introduce request handler
 */
static void serve_introduce(int fd, dictionary_t *query) {
  char *body;
  char buffer[MAXBUF];
  char send_buf[MAXLINE];
  char *status;
  char *version;
  char *desc;

  if (dictionary_count(query) != 4) {
    clienterror(fd, "POST", "400", "Bad Request",
                "Introduce takes four arguments only.");
    return;
  }

  const char *user = dictionary_get(query, "user");
  const char *friend = dictionary_get(query, "friend");
  char *host = (char *)dictionary_get(query, "host");
  char *port = (char *)dictionary_get(query, "port");
  
  if (!user || !friend || !host || !port) {
     clienterror(fd, "POST", "400", "Bad Request",
                "Bad argument(s).");
    return;
  }
  
  int connfd = Open_clientfd(host, port);
  sprintf(buffer, "GET /friends?user=%s HTTP/1.1\r\n\r\n", query_encode(friend));
  Rio_writen(connfd, buffer, strlen(buffer));
  Shutdown(connfd, SHUT_WR);
  rio_t rio;
  Rio_readinitb(&rio, connfd);

  if (Rio_readlineb(&rio, send_buf, MAXLINE) <= 0) {
    clienterror(fd, "POST", "400", "Bad Request",
                "Unable to read from requested server.");
  }
 
  if (!parse_status_line(send_buf, &version, &status, &desc)) {
    clienterror(fd, "POST", "400", "Bad Request",
                "Unable to get status.");
  }
  else {
    if (strcasecmp(version, "HTTP/1.0") && strcasecmp(version, "HTTP/1.1")) {
      clienterror(fd, version, "501", "Not Implemented",
                "Version not implemented in friendlist.");
    }
    else if (strcasecmp(status, "200") && strcasecmp(desc, "OK")) {
      clienterror(fd, status, "501", "Not Implemented",
                "Status not 'OK'.");
    }
    else {
      dictionary_t *hdrs = read_requesthdrs(&rio);
      char *len_str = dictionary_get(hdrs, "Content-length");
      int len = (len_str ? atoi(len_str) : 0);
      char r_buf[len];

      printf("len = %d\n", len);

      if (len <= 0) {
	clienterror(fd, "GET", "400", "Bad Request",
                "Unable to get friends as none received.");
      }
      else {
	print_stringdictionary(hdrs);
	Rio_readnb(&rio, r_buf, len);
	r_buf[len] = 0;

	pthread_mutex_lock(&lock);
	dictionary_t *friend_d = dictionary_get(friendlist, user);

	if (friend_d == NULL) {
	  friend_d = make_dictionary(COMPARE_CASE_SENS, NULL);
	  dictionary_set(friendlist, user, friend_d);
	}
	char **users_friends = split_string(r_buf, '\n');

	int i;
	for (i = 0; users_friends[i] != NULL; ++i) {
	  if (strcmp(users_friends[i], user) == 0)
	    continue;

	  dictionary_t *reg_user = (dictionary_t *)dictionary_get(friendlist, user);

	  if (reg_user == NULL) {
	    reg_user = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);
	    dictionary_set(friendlist, user, reg_user);
	  }

	  if (dictionary_get(reg_user, users_friends[i]) == NULL) {
	    dictionary_set(reg_user, users_friends[i], NULL);
	  }

	  dictionary_t *new_friend = (dictionary_t *)dictionary_get(friendlist, users_friends[i]);

	  if (new_friend == NULL) {
	    new_friend = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);
	    dictionary_set(friendlist, users_friends[i], new_friend);
	  }

	  if (dictionary_get(new_friend, user) == NULL) {
	    dictionary_set(new_friend, user, NULL);
	  }

	  free(users_friends[i]);
	}
	
	free(users_friends);

	const char **friend_names = dictionary_keys(friend_d);

	body = join_strings(friend_names, '\n');
	pthread_mutex_unlock(&lock);
	redirect_request(fd, body);
	free(body);
      }
    }

    free(version);
    free(status);
    free(desc);
  }

  Close(connfd);
}


/*
 * serve_befriend - befriend request handler
 */
static void serve_befriend(int fd, dictionary_t *query) {
  char *body;
  
  if (query == NULL) {
    clienterror(fd, "POST", "400", "Bad Request",
                "Query is Null.");
    return;
  }

  if (dictionary_count(query) != 2) {
    clienterror(fd, "POST", "400", "Bad Request",
                "/befriend takes only two arguments.");
  }

  const char *user = (char *)dictionary_get(query, "user");

  if (user == NULL) {
    dictionary_t *reg_user = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);
    dictionary_set(friendlist, user, reg_user);
  }

  dictionary_t *friend_d = (dictionary_t *)dictionary_get(friendlist, user);

  if (friend_d == NULL) {
    friend_d = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);
    dictionary_set(friendlist, user, friend_d);
  }

  char **users_friends = split_string((char *)dictionary_get(query, "friends"), '\n');

  if (users_friends == NULL) {
     clienterror(fd, "POST", "400", "Bad Request",
		"User is NULL.");
  }

  int i;
  for (i = 0; users_friends[i] != NULL; ++i) {

    if (strcmp(users_friends[i], user) == 0) {
      continue;
    }

    dictionary_t *reg_user = (dictionary_t *)dictionary_get(friendlist, user);

    if (reg_user == NULL) {
      reg_user = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);
      dictionary_set(friendlist, user, reg_user);
    }

    if (dictionary_get(reg_user, users_friends[i]) == NULL) {
      dictionary_set(reg_user, users_friends[i], NULL);
    }

    dictionary_t *new_friend = (dictionary_t *)dictionary_get(friendlist, users_friends[i]);
    
    if (new_friend == NULL) {
      new_friend = (dictionary_t *)make_dictionary(COMPARE_CASE_SENS, free);
      dictionary_set(friendlist, users_friends[i], new_friend);
    }

    if (dictionary_get(new_friend, user) == NULL) {
      dictionary_set(new_friend, user, NULL);
    }
  }

  friend_d = (dictionary_t *)dictionary_get(friendlist, user);
  const char **friend_names = dictionary_keys(friend_d);

  body = join_strings(friend_names, '\n');
  redirect_request(fd, body);
}


/* 
 * serve_unfriend - unfriend request handler
 */
static void serve_unfriend(int fd, dictionary_t *query) {
  char *body;

  if (dictionary_count(query) != 2) {
    clienterror(fd, "POST", "400", "Bad Request",
                "/unfriend takes only two arguments.");
  }

  const char *user = (char *)dictionary_get(query, "user");

  if (user == NULL) {
    clienterror(fd, "POST", "400", "Bad Request",
                "User is NULL.");
  }

  dictionary_t *friend_d = (dictionary_t *)dictionary_get(friendlist, user);

  if (friend_d == NULL) {
    clienterror(fd, "POST", "400", "Bad Request",
                "User is NULL.");
  }

  char **unfriend_users = split_string((char *)dictionary_get(query, "friends"), '\n');

  if (unfriend_users == NULL) {
    clienterror(fd, "GET", "400", "Bad Request",
                "Cannot get user's friends to unfriend.");
  }

  int i;
  for (i = 0; unfriend_users[i] != NULL; ++i) {
    dictionary_remove(friend_d, unfriend_users[i]);
    dictionary_t *unfriended_d = (dictionary_t *)dictionary_get(friendlist,unfriend_users[i]);

    if (unfriended_d != NULL) {
      dictionary_remove(unfriended_d, user);
    }
  }

  friend_d = (dictionary_t *)dictionary_get(friendlist, user);
  const char **friend_names = dictionary_keys(friend_d);

  body = join_strings(friend_names, '\n');
  redirect_request(fd, body);
}


/*
 * read_requesthdrs - read HTTP request headers
 */
dictionary_t *read_requesthdrs(rio_t *rp) {
  char buf[MAXLINE];
  dictionary_t *d = make_dictionary(COMPARE_CASE_INSENS, free);

  Rio_readlineb(rp, buf, MAXLINE);
  printf("%s", buf);
  while(strcmp(buf, "\r\n")) {
    Rio_readlineb(rp, buf, MAXLINE);
    printf("%s", buf);
    parse_header_line(buf, d);
  }
  
  return d;
}


/*
 * read_postquery - 
 */
void read_postquery(rio_t *rp, dictionary_t *headers, dictionary_t *dest) {
  char *len_str, *type, *buffer;
  int len;
  
  len_str = dictionary_get(headers, "Content-Length");
  len = (len_str ? atoi(len_str) : 0);

  type = dictionary_get(headers, "Content-Type");
  
  buffer = malloc(len+1);
  Rio_readnb(rp, buffer, len);
  buffer[len] = 0;

  if (!strcasecmp(type, "application/x-www-form-urlencoded")) {
    parse_query(buffer, dest);
  }

  free(buffer);
}


/*
 * serve_request - example request handler
 */
//static void serve_request(int fd, dictionary_t *query) {
//  size_t len;
//  char *body, *header;
//
//  body = strdup("alice\nbob");
//
//  len = strlen(body);
//
//  /* Send response headers to client */
//  header = ok_header(len, "text/html; charset=utf-8");
//  Rio_writen(fd, header, strlen(header));
//  printf("Response headers:\n");
//  printf("%s", header);
//
//  free(header);
//
//  /* Send response body to client */
//  Rio_writen(fd, body, len);
//
//  free(body);
//}


/*
 * clienterror - returns an error message to the client
 */
void clienterror(int fd, char *cause, char *errnum, 
		 char *shortmsg, char *longmsg) {
  size_t len;
  char *header, *body, *len_str;

  body = append_strings("<html><title>Friendlist Error</title>",
                        "<body bgcolor=""ffffff"">\r\n",
                        errnum, " ", shortmsg,
                        "<p>", longmsg, ": ", cause,
                        "<hr><em>Friendlist Server</em>\r\n",
                        NULL);
  len = strlen(body);

  /* Print the HTTP response */
  header = append_strings("HTTP/1.0 ", errnum, " ", shortmsg, "\r\n",
                          "Content-type: text/html; charset=utf-8\r\n",
                          "Content-length: ", len_str = to_string(len), "\r\n\r\n",
                          NULL);
  free(len_str);
  
  Rio_writen(fd, header, strlen(header));
  Rio_writen(fd, body, len);

  free(header);
  free(body);
}


/*
 * print_stringdictionary -
 */
static void print_stringdictionary(dictionary_t *d) {
  int i, count;

  count = dictionary_count(d);
  for (i = 0; i < count; i++) {
    printf("%s=%s\n",
           dictionary_key(d, i),
           (const char *)dictionary_value(d, i));
  }
  printf("\n");
}

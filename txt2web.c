/*
 * File: txt2web.c
 * ---------------
 * Original Author: Mason Armand
 * Contributors:
 * Date Created: May 10, 2023
 * Last Modified: May 15, 2023
 */
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

typedef struct {
        char* filename;
        char* title;
        char* date_str;
        char* description;
        time_t date;
} Post;

Post txt_to_html(const char* input_filename, const char* output_filename, bool add_link);
bool direxists(const char* dir);
void cleandirname(char* str);
void free_post(Post* post);
bool hasperms(const char* dir);
void copy_dir(const char* src, char* dest);
void remove_dir(const char* dirpath);
void file_warning(const char* err, const char* filename);
void parse_date(const char* date_str, time_t* result);
int compare_dates(const void* a, const void* b);
void remove_extension(const char* filename, char* output);
char* replace_links(const char* str);
char* replace_str(char* str, const char* find, const char* replace);
bool str_starts_with(const char* str, const char* prefix);
int str_starts_with_count(const char* str, const char prefix);
void clean_str(char* str);
char* str_repl_keywords(char* str, Post blog_post);
char* str_get_value(const char* str, const char* substring);
char* str_get_value_after_token(const char* str, const char* tok);


int main(int argc, char **argv)
{
        FILE* index;
        char line[1024];
        DIR* dir;
        struct dirent* ent;
        Post files[512];
        Post index_post;
        int file_count = 0;
        long index_pos;
        int i;
        char postdir[512];
        char indexloc[512];

        if (argc == 3) {
                if (strstr(argv[1], ".txt") == NULL) {
                        fprintf(stderr, "Usage: %s <input text file> <output file>\n", argv[0]);
                        return 0;
                }

                printf("Processing %s\n", argv[1]);
                txt_to_html(argv[1], argv[2], false);
                return 0;
        }
        else if (argc != 2) {
                fprintf(stderr, "Usage: %s <destination_directory>\n", argv[0]);
                return 0;
        }

        cleandirname(argv[1]);
        if (!direxists(argv[1])) {
                mkdir(argv[1], 0755);
        }

        if (!hasperms(argv[1])) {
                fprintf(stderr, "Write or read permission not granted for directory: %s\n", argv[1]);
                return 1;
        }

        remove_dir(argv[1]);
        printf("Copying files into build directory: %s\n", argv[1]);
        copy_dir(".", argv[1]);
        sprintf(postdir, "%s/posts/", argv[1]);
        sprintf(indexloc, "%s/index.html", argv[1]);

        mkdir(argv[1], 0755);
        mkdir(postdir, 0755);

        if ((dir = opendir(".")) == NULL) {
                fprintf(stderr, "Could not open directory");
                return 1;
        }

        while ((ent = readdir(dir)) != NULL) {
                char new_filename[273];
                char output_file[789];

                if (strstr(ent->d_name, ".txt") == NULL)
                        continue;

                printf("Processing %s\n", ent->d_name);
                remove_extension(ent->d_name, new_filename);

                sprintf(output_file, "%s%s.html", postdir, new_filename);
                files[file_count] = txt_to_html(ent->d_name, output_file, true);
                files[file_count].filename = strdup(new_filename);
                file_count++;
        }
        closedir(dir);

        /* sort posts by date */
        qsort(files, file_count, sizeof(Post), compare_dates);

        /* process index file */
        printf("Processing index.html\n");
        index_post = txt_to_html("index", indexloc, false);

        index = fopen(indexloc, "r+");
        if (index == NULL) {
                fprintf(stderr, "Could not open index.html\n");
                return 1;
        }

        while (fgets(line, sizeof(line), index)) {
                if (strstr(line, "</main>") != NULL) {
                        index_pos = ftell(index) - strlen(line);
                        break;
                }
        }

        fseek(index, index_pos, SEEK_SET);
        fprintf(index, "  <nav><ul>\n");
        for (i = 0; i < file_count; i++) {
                fprintf(index, "    <li><span class='date'>%s</span> - <a href='posts/%s.html'>%s</a></li>\n", files[i].date_str, files[i].filename, files[i].title);
                free_post(&files[i]);
        }

        fprintf(index, "  </ul></nav>\n</main>\n</body>\n</html>\n");
        fclose(index);
        free_post(&index_post);

        return 0;
}


Post txt_to_html(const char* input_filename, const char* output_filename, bool add_link)
{
        FILE* f_in = fopen(input_filename, "r");
        FILE* f_out = fopen(output_filename, "w");
        char line[1024];
        bool in_paragraph = false;
        bool in_code = false;
        bool in_meta = false;
        bool has_tags = false;
        Post blog_post = { 0 };

        if (f_in == NULL) {
                fprintf(stderr, "ERROR: Trying to write to nonexistent file: %s", input_filename);
                exit(1);
        }

        fprintf(f_out, "<!DOCTYPE html>\n<html lang='en'>\n<head>\n");
        fprintf(f_out, "  <meta charset='UTF-8'>\n");
        fprintf(f_out, "  <meta name='viewport' content='width=device-width, initial-scale=1'>\n");
        fprintf(f_out, "  <link href='/style.css' rel='stylesheet' type='text/css' media='all'>\n");

        while(fgets(line, sizeof(line), f_in)) {
                /* META */
                if (str_starts_with(line, "-----") && !in_meta && !in_code) {
                        in_meta = true;
                }
                else if (str_starts_with(line, "-----") && !in_code) {
                        in_meta = false;
                        fprintf(f_out, "\n</head>\n<body>\n<main>\n");
                        if (add_link) {
                                fprintf(f_out, "<a href='/'>&lt;-- go to home page</a><br><br>");
                        }
                        continue;
                }
                if (in_meta && !in_code) {
                        if (str_starts_with(line, "title:")) {
                                blog_post.title = str_get_value(line, "title:");
                                fprintf(f_out, "  <title>%s</title>", blog_post.title);
                        }
                        else if (str_starts_with(line, "date:")) {
                                blog_post.date_str = str_get_value(line, "date:");
                                parse_date(blog_post.date_str, &blog_post.date);
                        }
                        else if (str_starts_with(line, "description:")) {
                                blog_post.description = str_get_value(line, "description:");
                                fprintf(f_out, "\n  <meta name='description' content='%s'>", blog_post.description);
                        }
                        else if (str_starts_with(line, "tags:")) {
                                char* str_tags = str_get_value(line, "tags:");
                                fprintf(f_out, "\n  <meta name='keywords' content='%s'>", str_tags);
                                free(str_tags);
                                has_tags = true;
                        }
                        else if (str_starts_with(line, "style:")) {
                                char* str_style = str_get_value(line, "style:");
                                fprintf(f_out, "\n  <link href='%s' rel='stylesheet' type='text/css' media='all'>", str_style);
                                free(str_style);
                        }
                }
                /* code blocks */
                else if (in_code) {
                        if (str_starts_with(line, "```")) {
                                fprintf(f_out, "</code></pre>\n");
                                in_code = false;
                        }
                        else {
                                char* rpl_less = replace_str(line, "<", "&lt;");
                                char* rpl_greater = replace_str(rpl_less, ">", "&gt;");
                                fprintf(f_out, "%s", rpl_greater);
                                free(rpl_less);
                                free(rpl_greater);
                        }
                }
                /* Images */
                else if (str_starts_with(line, "@")) {
                        char* str_img = str_get_value(line, "@");
                        fprintf(f_out, "  <img src='%s'>\n", str_img);
                        free(str_img);
                }
                /* headings */
                else if (str_starts_with(line, "#")) {
                        int header_level = str_starts_with_count(line, '#');
                        char* str = str_get_value_after_token(line, "#");
                        char* rpl = str_repl_keywords(str, blog_post);
                        char* link_rpl = replace_links(rpl);
                        fprintf(f_out, "  <h%d>%s</h%d>\n", header_level, link_rpl, header_level);
                        free(str);
                        free(rpl);
                        free(link_rpl);
                }
                else if (str_starts_with(line, "```")) {
                        if (in_paragraph)
                                fprintf(f_out, "  </p>\n");
                        fprintf(f_out, "<pre><code>");
                        in_code = true;
                        in_paragraph = false;
                }
                /* Paragraph tags */
                else if (in_paragraph && strlen(line) <= 1) {
                        fprintf(f_out, "  </p>\n");
                        in_paragraph = false;
                }
                else if (!in_paragraph && strlen(line) > 1 && strstr(line, "<") == NULL) {
                        char* str = str_repl_keywords(line, blog_post);
                        char* rpl = replace_links(str);
                        fprintf(f_out, "\n  <p>\n    %s", rpl);
                        in_paragraph = true;
                        free(str);
                        free(rpl);
                }
                else {
                        char* str = str_repl_keywords(line, blog_post);
                        char* rpl = replace_links(str);
                        fprintf(f_out, "    %s", rpl);
                        free(str);
                        free(rpl);
                }
        }

        if (in_paragraph) {
                fprintf(f_out, "  </p>\n");
        }
        fprintf(f_out, "</main>\n</body>\n</html>\n");

        /* Errors and warnings */
        if (blog_post.date_str == NULL) {
                fprintf(stderr, "Error: Document date not set in %s. Please set the date in your document.", input_filename);
        }
        if (blog_post.title == NULL) {
                file_warning("Document title not set. Title will be the name of the document filename.", input_filename);
                remove_extension(input_filename, blog_post.title);
        }
        if (blog_post.description == NULL) {
                file_warning("Document description not set.", input_filename);
        }
        if (!has_tags) {
                file_warning("Document tags not set.", input_filename);
        }

        fclose(f_in);
        fclose(f_out);

        return blog_post;
}


bool direxists(const char* dir)
{
        struct stat st = { 0 };
        return !(stat(dir, &st) == -1);
}


void cleandirname(char* str)
{
        size_t len = strlen(str);
        if (str[len - 1] == '/')
                str[len - 1] = '\0';
}


void free_post(Post* post)
{
        free(post->title);
        free(post->description);
        free(post->date_str);
        free(post->filename);
}


bool hasperms(const char* dir)
{
        return (access(dir, R_OK | W_OK) == 0);
}


void copy_dir(const char* src, char* dest)
{
        DIR* dir = opendir(src);
        struct dirent* entry;
        char src_path[1024];
        char dest_path[1024];
        struct stat st;

        if (dir == NULL)
                return;
        mkdir(dest, 0755);


        while ((entry = readdir(dir)) != NULL) {
                bool is_dir = strcmp(entry->d_name, ".") == 0;
                bool is_pdir = strcmp(entry->d_name, "..") == 0;
                bool is_index = strcmp(entry->d_name, "index") == 0;
                bool is_dest = strcmp(entry->d_name, dest) == 0;
                bool is_sh = strstr(entry->d_name, ".sh") != NULL;
                bool is_txt = strstr(entry->d_name, ".txt") != NULL;

                if (is_dir || is_pdir || is_index || is_dest || is_sh || is_txt)
                        continue;

                snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
                snprintf(dest_path, sizeof(dest_path), "%s/%s", dest, entry->d_name);

                if (stat(src_path, &st) == -1)
                        continue;

                if (S_ISDIR(st.st_mode)) {
                        copy_dir(src_path, dest_path);
                }
                else if (S_ISREG(st.st_mode)) {
                        FILE* src_file = fopen(src_path, "rb");
                        FILE* dest_file = fopen(dest_path, "wb");

                        char buf[1024];
                        size_t size;

                        printf("Copying file: %s to %s\n", src_path, dest_path);

                        while ((size = fread(buf, 1, sizeof(buf), src_file)) > 0) {
                                fwrite(buf, 1, size, dest_file);
                        }

                        fclose(src_file);
                        fclose(dest_file);
                }
        }
        closedir(dir);
}


void remove_dir(const char* dirpath)
{
        DIR* dir = opendir(dirpath);
        struct dirent* entry;
        char path[1024];
        struct stat st;

        if (dir == NULL)
                return;

        while ((entry = readdir(dir)) != NULL) {
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                        continue;
                }

                snprintf(path, sizeof(path), "%s/%s", dirpath, entry->d_name);
                if (stat(path, &st) == -1) {
                        continue;
                }

                if (S_ISDIR(st.st_mode)) {
                        remove_dir(path);
                        rmdir(path);
                }
                else if (S_ISREG(st.st_mode)) {
                        remove(path);
                }
        }
        closedir(dir);
}


void file_warning(const char* err, const char* filename)
{
        fprintf(stderr, "WARNING: %s (%s)\n", err, filename);
}


void parse_date(const char* date_str, time_t* result)
{
        struct tm tm_date = { 0 };
        strptime(date_str, "%b %d %Y", &tm_date);
        *result = mktime(&tm_date);
}


int compare_dates(const void* a, const void* b)
{
        Post* blog_a = (Post*) a;
        Post* blog_b = (Post*) b;

        if (blog_a->date > blog_b->date)
                return -1;
        else if (blog_a->date < blog_b->date)
                return 1;
        else
                return 0;
}


void remove_extension(const char* filename, char* output)
{
        char *dot = strrchr(filename, '.');
        if (dot) {
                strncpy(output, filename, dot - filename);
                output[dot - filename] = '\0';
        }
}


char* replace_str(char* str, const char* find, const char* replace)
{
        char* result;
        char* ins; /* next insert point */
        char* tmp;
        int len_find;
        int len_replace;
        int len_front; /* distance between ins and end of the last match */
        int count;

        if (!str || !find || !replace)
                return NULL;
        len_find = strlen(find);
        if (len_find == 0)
                return NULL;
        len_replace = strlen(replace);

        /* count number of needed replacements */
        ins = str;
        for (count = 0; (tmp = strstr(ins, find)); ++count) {
                ins = tmp + len_find;
        }

        tmp = result = malloc(strlen(str) + (len_replace - len_find) * count + 1);

        if (!result)
                return NULL;

        while (count--) {
                ins = strstr(str, find);
                len_front = ins - str;
                tmp = strncpy(tmp, str, len_front) + len_front;
                tmp = strcpy(tmp, replace) + len_replace;
                str += len_front + len_find; /* move to end of next match */
        }
        strcpy(tmp, str);
        return result;
}


char* replace_links(const char* str)
{
        const char* http = "http://";
        const char* https = "https://";
        const char* start = str;
        const char* pos = NULL;
        char* result = malloc(strlen(str) * 3 + strlen("<a href=''></a>") + 1);
        char* result_pos = result;

        if (!result)
                return NULL;

        while ((pos = strstr(start, http)) || (pos = strstr(start, https))) {
                const char* end = pos;
                char* url;
                size_t url_len;

                memcpy(result_pos, start, pos - start);
                result_pos += pos - start;

                while (*end && !isspace(*end))
                        end++;

                url_len = end - pos;
                url = malloc(url_len + 1);

                if (!url) {
                        free(result);
                        return NULL;
                }

                strncpy(url, pos, url_len);
                url[url_len] = '\0';

                result_pos += sprintf(result_pos, "<a href='%s'>%s</a>", url, url);
                free(url);
                start = end;
        }
        strcpy(result_pos, start);
        return result;
}


int str_starts_with_count(const char* str, const char prefix)
{
        int count = 0;
        size_t i;

        for (i = 0; i < strlen(str); i++) {
                if (str[i] != prefix) {
                        break;
                }
                count++;
        }

        return count;
}


bool str_starts_with(const char* str, const char* prefix)
{
        return strncmp(prefix, str, strlen(prefix)) == 0;
}


void clean_str(char* str)
{
        /* find first non-whitespace char */
         while(isspace((unsigned char)*str)) str++;

         /* create new string without the leading whitespace */
         memmove(str, str, strlen(str) + 1);

         str[strcspn(str, "\n")] = 0; /* remove newline */
}


char* str_repl_keywords(char* str, Post blog_post)
{
        char* result;
        char* final_result;
        /* replace {title} and {date} with their respective values */
        result = replace_str(str, "{title}", blog_post.title);
        final_result = replace_str(result, "{date}", blog_post.date_str);
        free(result);
        return final_result;
}


char* str_get_value(const char* str, const char* substring)
{
        char* substr_in_str = strstr(str, substring);
        size_t start_pos = substr_in_str - str + strlen(substring);
        char* value = malloc(strlen(str) - start_pos + 1);

        strcpy(value, &str[start_pos]);
        clean_str(value);
        return value;
}


char* str_get_value_after_token(const char* str, const char* tok)
{
        char* str_cpy = strdup(str);
        char* value = strtok(str_cpy, tok);
        char* result;

        if (!value) {
                fprintf(stderr, "Error when calling strtok, str_get_value_after_token\n");
                free(str_cpy);
                return NULL;
        }
        clean_str(value);
        result = strdup(value);
        free(str_cpy);

        return result;
}

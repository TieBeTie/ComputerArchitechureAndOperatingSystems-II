#include <string.h>
#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <inttypes.h>

#define FUSE_USE_VERSION 30
#include <fuse.h>

char *REAL_PATH;
FILE *DATA_FILE;
uint64_t COUNT;
char **FILE_NAME;
uint64_t *FILE_SIZE;
uint64_t *FILE_HEAD;
uint64_t FILE_POS;
char BUFFER[100000];

typedef struct {
  char *arg_file;
} my_options_t;
my_options_t my_options;

void print_cwd() {
  if (my_options.arg_file) {
    FILE *f = fopen("error.txt", "at");
    char buffer[1000];
    getcwd(buffer, sizeof(buffer));
    if (REAL_PATH == NULL) {
      REAL_PATH = (char *) malloc(PATH_MAX * sizeof(char));
      memccpy(REAL_PATH, buffer, '\0', strlen(buffer));
      realpath(buffer, REAL_PATH);
    }

    fprintf(f, "Current working dir: %s\n", buffer);
    fprintf(f, "real %s\n", REAL_PATH);
    fclose(f);
  }
}

// Самый важный колбэк. Вызывается первым при любом другом колбэке.
// Заполняет структуру stbuf.
int getattr_callback(const char *path, struct stat *stbuf, struct fuse_file_info *fi) {
  (void) fi;
  strncat(REAL_PATH, path, PATH_MAX + 1);
  realpath(REAL_PATH, REAL_PATH);
  if (strcmp(path, "/") == 0) {
    // st_mode(тип файла, а также права доступа)
    // st_nlink(количество ссылок на файл)
    // Интересный факт, что количество ссылок у папки = 2 + n, где n -- количество подпапок.
    *stbuf = (struct stat) {
        .st_nlink = 2,
        .st_mode = S_IFDIR | 0555
    };
    return 0;
  }
  for (uint64_t i = 0; i < COUNT; ++i) {
    if (REAL_PATH[0] == '/' && strcmp(REAL_PATH + 1, FILE_NAME[i]) == 0) {
      *stbuf = (struct stat) {
          .st_nlink = 1,  //?
          .st_mode = S_IFREG | 0444,
          .st_size = (__off_t) FILE_SIZE[i]
      };
      return 0;
    }
  }
  return -ENOENT; // При ошибке, вместо errno возвращаем (-errno).
}
//
// filler(buf, filename, stat, flags) -- заполняет информацию о файле и вставляет её в buf.
int readdir_callback(
    const char *path, void *buf,
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info *fi,
    enum fuse_readdir_flags flags
) {
  (void) offset;
  (void) fi;
  (void) flags; // unused variables
  filler(buf, ".", NULL, 0, (enum fuse_fill_dir_flags) 0);
  filler(buf, "..", NULL, 0, (enum fuse_fill_dir_flags) 0);
  for (uint64_t i = 0; i < COUNT; ++i) {
    filler(buf, FILE_NAME[i], NULL, 0, (enum fuse_fill_dir_flags) 0);
  }
  return 0;
}

//// Вызывается после успешной обработки open.
int read_callback(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi) {
  // "/"
  if (strcmp(path, "/") == 0) {
    return -EISDIR;
  }
  //print_cwd();
  // "/my_file"
  for (uint64_t i = 0; i < COUNT; ++i) {
    if (path[0] == '/' && strcmp(path + 1, FILE_NAME[i]) == 0) {
      if (offset >= FILE_SIZE[i]) {
        return 0;
      }
      size = (offset + size <= FILE_SIZE[i]) ? size : (FILE_SIZE[i] - offset);

      fseek(DATA_FILE, (FILE_HEAD[i] + offset) * sizeof(char), SEEK_CUR);
      for (size_t j = 0; j < size; ++j) {
        BUFFER[j + offset] = fgetc(DATA_FILE);
      }

      fseek(DATA_FILE, -1 * (FILE_HEAD[i] + offset + size) * sizeof(char), SEEK_CUR);
      memcpy(buf, BUFFER + offset, size);
      return size;
    }
  }

return -EIO;
}

// Структура с колбэками.
struct fuse_operations fuse_example_operations = {
    .getattr = getattr_callback,
    .read = read_callback,
    .readdir = readdir_callback,
};

struct fuse_opt opt_specs[] = {
    {"--src %s", offsetof(my_options_t, arg_file), 0},
    FUSE_OPT_END // Структурка заполненная нулями. В общем такой типичный zero-terminated массив
};

int main(int argc, char **argv) {
  struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
  /*
  * ВАЖНО: заполняемые поля должны быть инициализированы нулями.
  * (В противном случае fuse3 может делать что-то очень плохое. TODO)
  */
  fuse_opt_parse(&args, &my_options, opt_specs, NULL);
  print_cwd();
  //init
  DATA_FILE = fopen(my_options.arg_file, "r");
  fscanf(DATA_FILE, "%ld", &COUNT);

  FILE_SIZE = (uint64_t *) malloc((COUNT) * sizeof(uint64_t));
  //allocate names
  FILE_NAME = (char **) malloc((COUNT) * sizeof(char *));

  for (uint64_t i = 0; i < COUNT; ++i) {
    FILE_NAME[i] = (char *) malloc((FILENAME_MAX + 1) * sizeof(char));
    fscanf(DATA_FILE, "%s %ld", FILE_NAME[i], FILE_SIZE + i);
  }

  //skip \n
  fseek(DATA_FILE, 2, SEEK_CUR);

  //FILE HEADS INIT
  FILE_HEAD = (uint64_t *) malloc((COUNT) * sizeof(uint64_t));
  FILE_HEAD[0] = 0;

  for (uint64_t i = 1; i < COUNT; ++i) {
    FILE_HEAD[i] = FILE_HEAD[i - 1] + FILE_SIZE[i - 1];
  }

  int ret = fuse_main(args.argc, args.argv, &fuse_example_operations, NULL);
  fseek(DATA_FILE, 8 * sizeof(char), SEEK_CUR);
  for (int i = 0; i < 10; ++i) {
    char buffer;
    buffer = fgetc(DATA_FILE);
    printf("%c", buffer);
  }
  fseek(DATA_FILE, 8 * sizeof(char), SEEK_CUR);
  for (int i = 0; i < 10; ++i) {
    char buffer;
    buffer = fgetc(DATA_FILE);
    printf("%c", buffer);
  }

  fuse_opt_free_args(&args);
  fclose(DATA_FILE);
  for (uint64_t i = 0; i < COUNT; ++i) {
    free(FILE_NAME[i]);
  }
  free(FILE_NAME);
  free(FILE_SIZE);
  free(REAL_PATH);
  // return ret;
}
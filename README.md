# txt2web
A simple static site generator for creating a blog. See it in action here: https://blog.masonarmand.com/

This simple c program turns a directory of plaintext files into a full html website.
The only dependencies are the C standard library and POSIX functions.
Here is the syntax for creating a blog post (txt file):
``````
-----
title: Your Blog Title
date: May 11 2023
description: This is the HTML meta description
tags: these, are, comma, seperated, meta, keywords
style: custom.css
-----
# Markdown style headings
## heading size depends on number of pound signs

Images can be added using an @ sign and a filename:
@/images/image.png

Code blocks can also be written like in markdown:
```
int main(void)
{
        printf("This is a code block!\n");
        return 0;
}
```
``````
Also, you need to create a file called `index` with the same format as above to generate the homepage. At the end of the index file, the list of blog posts will be automatically added (sorted by date). Any files in the source directory will also be copied (recursively), such as images and stylesheets. Also by default every blog post will look for a style.css in the root directory. You can specify a specific css file using `style: filename.css` in the blog heading.

## Usage
To generate your website simply enter this command
`txt2web <build_directory>`
replacing `<build_directory>` with the directory that the website will generate to. Please note, that the build directory will be cleared before building the site, so do not make the mistake of making the build directory the same as the source directory.

## Todo
- Add formatting for:
  - lists
  - inline code blocks (one backtick)
- References using `[number]` with bibliography at the end of the page?

## Installation
```
git clone https://github.com/masonarmand/txt2web.git
cd txt2web
sudo make install
```

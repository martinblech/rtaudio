augroup autoformat_settings
  autocmd FileType c,cpp,proto AutoFormatBuffer clang-format
  autocmd FileType javascript,typescript,typescript.tsx AutoFormatBuffer prettier
  autocmd FileType html,css,sass,scss,less,json AutoFormatBuffer js-beautify
augroup END

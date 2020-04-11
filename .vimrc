set autoindent
set mouse=a
syntax on
set smartindent
set number
set ruler
set noet ci pi sts=0 sw=4 ts=4
call plug#begin('~/.vim/bundle')
Plug 'color/vim-one'
call plug#end()
colorscheme one
set background=dark
set colorcolumn=81
highlight colorcolumn guibg=red ctermbg=red
highlight ExtraWhitespace ctermbg=red guibg=red
match ExtraWhitespace /\s\+$/
autocmd Syntax * syn match ExtraWhitespace /\s\+$\| \+\ze\t/

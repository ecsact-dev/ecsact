vim.lsp.set_log_level("TRACE")

local cmd = vim.fn.getcwd() .. "/bazel-bin/ecsact_lsp_server"

if vim.fn.has('win32') then
	cmd = cmd .. ".exe"
end

local diagnostics_examples = vim.split(
	vim.fn.glob(vim.fn.getcwd() .. '/test/diagnostics_examples/*.ecsact'),
	'\n'
)

local current_index = 0

local function open_example()
	local file = diagnostics_examples[current_index + 1]
	vim.cmd('edit ' .. file)
	vim.bo.filetype = "ecsact"

	vim.lsp.start({
		name = 'ecsact',
		cmd = {cmd, "--stdio"},
		root_dir = vim.fn.getcwd(),
	})
end

vim.api.nvim_set_keymap('n', 'n', '', {
	noremap = true,
	callback = function()
		current_index = (current_index + 1) % #diagnostics_examples
		open_example()
	end,
})

vim.api.nvim_set_keymap('n', 'N', '', {
	noremap = true,
	callback = function()
		current_index = (current_index - 1) % #diagnostics_examples
		open_example()
	end,
})

vim.api.nvim_set_keymap('n', 'q', '', {
	noremap = true,
	callback = function()
		vim.cmd('qa!')
	end,
});

open_example()


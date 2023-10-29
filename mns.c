/*
gcc -I/opt/homebrew/Cellar/readline/8.2.1/include -L/opt/homebrew/Cellar/readline/8.2.1/lib -o mns mns.c libft/libft.a -lreadline -lncurses
gcc -I/usr/local/Cellar/readline/8.2.1/include -L/usr/local/Cellar/readline/8.2.1/lib -o mns mns.c libft/libft.a -lreadline -lncurses
*/

/////////////////////////////////* * PLAN * *////////////////////////////////////
/*
	- readline
	- signal
	- unorganized_token <-- NOW (input_to_token, split_space)
	- organized_token
	- err_check
	- grouped_token
*/
/////////////////////////////////////////////////////////////////////////////////

#include "minishell.h"

////////////////////////////**DEBUG FUNCTION**////////////////////////////////////

// print token_node 
// token_mark อ่านเป็น index ex. m_undefined = 0, m_heredoc = 1,...
void print_token_node(t_token_node *node) {
if (node) {
	printf("Mark: %u\n", node->mark);
	printf("Type: %s\n", node->type); 
	printf("Value: %s\n", node->value); 
} else {
	printf("Node is NULL\n");
}
}

// print link list
void print_link_list(t_token_node *head) {
	t_token_node *current = head;
	while (current != NULL) {
		print_token_node(current);
		current = current->next;
	}
	printf("NULL\n");
}

/////////////////////////////////////////////////////////////////////////////////

t_token_node	*token_node_create (t_token_ptr *ptr, t_token_mark mark)
{
	t_token_node	*new;

	if(!ptr)
		return (NULL);
	new = (t_token_node *)malloc(sizeof(t_token_node) * 1);
	if (!new)
		return (NULL);
	new->mark = mark;
	new->type = 0;
	new->value = 0;
	new->next = 0;
	if (ptr->head)
	{
		(ptr->tail)->next = new;
		ptr->tail = (ptr->tail)->next;
		return (new);
	}
	else
	{
		ptr->head = new;
		ptr->tail = ptr->head;
		return (new);
	}
}

////////////////////////////input_to_token_utils2.c////////////////////////////////////

char	*match_outside_qoute(char *tmp_ptr, char *must_match, int match_in_dbq)
{
	int		sq;
	int		dbq;

	sq = 0;
	dbq = 0;
	while (*tmp_ptr)
	{
		if (!dbq && *tmp_ptr == '\'')
			sq = !sq;
		if (!sq && *tmp_ptr == '\"')
			dbq = !dbq;
		if (!ft_strncmp(tmp_ptr, must_match, ft_strlen(must_match)) && !sq && (!dbq || match_in_dbq))
			break ;
		tmp_ptr++;
	}
	return (tmp_ptr);
}

////////////////////////////input_to_token_utils1.c////////////////////////////////////

static void	split_space(t_data *data, t_token_ptr *tmp_ptr, t_token_node *src_head)
{
	char	*main_cptr;
	char	*match_cptr;
	size_t	len;

	(void)(data);
	main_cptr = src_head->value;
	match_cptr = src_head->value;
	while (1)
	{
		match_cptr = match_outside_qoute(match_cptr, " ", 0);
		len = match_cptr - main_cptr;
		if (len)
			token_node_create(tmp_ptr, src_head->mark)->value = ft_substr(main_cptr, 0, len);
		if (!*match_cptr)
			break ;
		match_cptr += 1;
		main_cptr = match_cptr;
	}
}

void	token_split(t_data *data, t_token_ptr *src, t_token_ptr *dst, void (*fn)(t_data *data, t_token_ptr *tmp_ptr, t_token_node *src_head))
{
	t_token_ptr		tmp_ptr;
	t_token_node	*head_ptr;

	tmp_ptr.head = 0;
	tmp_ptr.tail = 0;

	while (src->head)
	{
		if(!fn || !src || !dst || !src->head)
			return ;
		fn (data, &tmp_ptr, src->head);
		head_ptr = src->head;
		src->head = src->head->next;
		free(head_ptr->value);
		free(head_ptr);
		if(!tmp_ptr.head)
			return ;
		if (!dst->head)
			dst->head = tmp_ptr.head;
		else
			dst->tail->next = tmp_ptr.tail;
		dst->tail = tmp_ptr.tail;
	}
}

////////////////////////////input_to_token.c////////////////////////////////////

void	input_to_token(t_data *data, char *input)
{
	t_token_ptr	tmp1;
	t_token_ptr	tmp2;
	t_token_ptr	*output;

	output = &data->unorganized_token;
	if(!input || !output)
		return ;
	tmp1.head = 0;
	tmp1.tail = 0;
	tmp2.head = 0;
	tmp2.tail = 0;
	token_node_create(&tmp1, m_undefined)->value = input;
	// free(input); //ไม่แน่ใจว่าต้องฟรีมั้ย แต่เอาค่าไป = valueแล้ว ซึ่งจะฟรีvalueทีหลัง
	token_split (data, &tmp1, &tmp2, split_space);
	print_link_list(tmp2.head); // ** DEBUG **
}

////////////////////////////mns_free.c////////////////////////////////////

void	mns_free(t_data *data)
{
	rl_clear_history();
}

void	free_char_2d(char **ptr)
{
	int	i;

	i = 0;
	if (!ptr)
		return ;
	while (ptr[i])
	{
		free(ptr[i]);
		ptr[i] = NULL;
		i++;
	}
	free (ptr);
	ptr = NULL;
}

////////////////////////////mns_init.c////////////////////////////////////

static void	env_init(t_data *data, char **envp)
{
	int	i;
	int	row;

	i = 0;
	row = 0;
	while (envp[row])
		row++;
	data->env = (char **)malloc(sizeof(char *) * (row + 1));
	if(!data->env)
		return ;
	while (i < row)
	{
		data->env[i] = ft_strdup(envp[i]);
		if(!data->env[i])
		{
			free_char_2d(data->env);
			return ;
		}
		i++;
	}
	data->env[row] = 0;
	data->env_row_max = row;
}

static void	sig_quit_handling(int sig_num)
{
	if (sig_num == SIGINT && g_signal == 0)
		rl_redisplay();
}

static void	sig_int_handling(int sig_num)
{
	if (sig_num == SIGINT && g_signal == 0)
	{
		ft_putchar_fd('\n', STDOUT_FILENO);
		rl_on_new_line();
		rl_replace_line("", STDOUT_FILENO);
		rl_redisplay();
	}
}

static void	signal_init (t_data *data)
{
	t_sigaction	sig_int;
	t_sigaction	sig_quit;

	sig_int.sa_handler = sig_int_handling;
	sigemptyset(&sig_int.sa_mask);
	sig_int.sa_flags = SA_RESTART;
	sigaction(SIGINT, &sig_int, NULL);
	sig_quit.sa_handler = sig_quit_handling;
	sigemptyset(&sig_quit.sa_mask);
	sig_quit.sa_flags = SA_RESTART;
	sigaction(SIGQUIT, &sig_quit, NULL);
}

void	mns_init (t_data *data, char **envp)
{
	g_signal = 0;
	data->errnum = 0;
	data->env = 0;
	data->env_row_max = 0;
	data->unorganized_token.head = 0;
	data->unorganized_token.tail = 0;


	signal_init (data);
	env_init(data, envp);
}

////////////////////////////minishell.c////////////////////////////////////

char	*ft_strjoin_free(char *src, char *dst)
{
	char	*res;
	size_t	count;
	size_t	i;
	size_t	j;

	if (!dst && !src)
		return (NULL);
	if (!dst || !*dst)
		return (src);
	else if (!src || !*src)
		return (dst);
	count = ft_strlen(src) + ft_strlen(dst);
	res = (char *)malloc(sizeof(char) * (count + 1));
	if (!res)
		return (NULL);
	i = 0;
	j = 0;
	while(src[j])
		res[i++] = src[j++];
	j = 0;
	while (dst[j])
		res[i++] = dst[j++];
	res[i] = '\0';
	return (free(src), free(dst), res);
}

static char	*input_msg_init (t_data *data)
{
	char	*path_color;
	char	*err_color;
	char	*input_msg;
	char	path_dir[PATH_MAX];

	getcwd(path_dir, sizeof(path_dir));
	path_color = ft_strjoin_free (ft_strdup("\x1b[34m"), ft_strdup(path_dir));
	err_color = ft_strjoin_free(ft_strdup("\x1b[33m?"), ft_itoa(data->errnum));
	input_msg = ft_strjoin_free(ft_strjoin_free(path_color, err_color), ft_strdup("\x1b[34m $\x1b[0m "));
	// input_msg = ft_strjoin_free(ft_strjoin_free (ft_strdup("\x1b[34m"), ft_strdup(path_dir)), ft_strdup("$\x1b[0m "));
	if (!input_msg)
		input_msg = ft_strdup("minishell_input: ");
	return(input_msg);
}

static int	main_while (t_data *data)
{
	char	*input;
	char	*input_msg;

	input_msg = input_msg_init(data);
	input = readline(input_msg);
	free(input_msg);
	if (!input)
		return (1);
	if (!input[0])
	{
		free (input);
		return (0);
	}
	add_history(input);
	input_to_token(data, input);
	// token_to_organize(data);

	// // *np-sam* --> ต้องไปอยู่หลัง organize ก่อน err_check
	// if (!(data->organized_token))
	// {
	// 	mns_free(data);
	// 	exit (0);
	// }

	return(0);

}

int main(int argc, char **argv, char **envp) 
{
	t_data	data;

	(void) argv;
	if (argc != 1)
		return (0);
	mns_init(&data, envp);
	while (1)
	{
		if(main_while(&data))
			break ;
		g_signal = 1;
		// excute_cmd(&data);
		g_signal = 0;
	}
	printf ("exit\n");
	mns_free(&data);
	return (0);
}

////////////////////////////**DEBUG**////////////////////////////////////

// print env_init
	// for (i = 0; i < row; i++) {
	// 	printf("data->env[%d] = %s\n", i, data->env[i]);}

// print input
	// if (input) {
	// 	printf("You entered: %s\n", input);
	// 	free(input); 
	// }

// printf
	// printf("%s", "checked");
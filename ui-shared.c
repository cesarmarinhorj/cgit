/* ui-shared.c: common web output functions
 *
 * Copyright (C) 2006-2014 cgit Development Team <cgit@lists.zx2c4.com>
 *
 * Licensed under GNU General Public License v2
 *   (see COPYING for full license text)
 */

#include "cgit.h"
#include "ui-shared.h"
#include "cmd.h"
#include "html.h"

static const char cgit_doctype[] =
"<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Transitional//EN\"\n"
"  \"http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd\">\n";

static char *http_date(time_t t)
{
	static char day[][4] =
		{"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
	static char month[][4] =
		{"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	struct tm *tm = gmtime(&t);
	return fmt("%s, %02d %s %04d %02d:%02d:%02d GMT", day[tm->tm_wday],
		   tm->tm_mday, month[tm->tm_mon], 1900 + tm->tm_year,
		   tm->tm_hour, tm->tm_min, tm->tm_sec);
}

void cgit_print_error(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	cgit_vprint_error(fmt, ap);
	va_end(ap);
}

void cgit_vprint_error(const char *fmt, va_list ap)
{
	va_list cp;
	html("<div class='error'>");
	va_copy(cp, ap);
	html_vtxtf(fmt, cp);
	va_end(cp);
	html("</div>\n");
}

const char *cgit_httpscheme(void)
{
	if (ctx.env.https && !strcmp(ctx.env.https, "on"))
		return "https://";
	else
		return "http://";
}

const char *cgit_hosturl(void)
{
	if (ctx.env.http_host)
		return ctx.env.http_host;
	if (!ctx.env.server_name)
		return NULL;
	if (!ctx.env.server_port || atoi(ctx.env.server_port) == 80)
		return ctx.env.server_name;
	return fmtalloc("%s:%s", ctx.env.server_name, ctx.env.server_port);
}

const char *cgit_currenturl(void)
{
	if (!ctx.qry.url)
		return cgit_rooturl();
	const char *root = cgit_rooturl();
	size_t len = strlen(root);
	if (len && root[len - 1] == '/')
		return fmtalloc("%s%s", root, ctx.qry.url);
	return fmtalloc("%s/%s", root, ctx.qry.url);
}

const char *cgit_rooturl(void)
{
	if (ctx.cfg.virtual_root)
		return ctx.cfg.virtual_root;
	else
		return ctx.cfg.script_name;
}

const char *cgit_loginurl(void)
{
	static const char *login_url;
	if (!login_url)
		login_url = fmtalloc("%s?p=login", cgit_rooturl());
	return login_url;
}

char *cgit_repourl(const char *reponame)
{
	if (ctx.cfg.virtual_root)
		return fmtalloc("%s%s/", ctx.cfg.virtual_root, reponame);
	else
		return fmtalloc("?r=%s", reponame);
}

char *cgit_fileurl(const char *reponame, const char *pagename,
		   const char *filename, const char *query)
{
	struct strbuf sb = STRBUF_INIT;
	char *delim;

	if (ctx.cfg.virtual_root) {
		strbuf_addf(&sb, "%s%s/%s/%s", ctx.cfg.virtual_root, reponame,
			    pagename, (filename ? filename:""));
		delim = "?";
	} else {
		strbuf_addf(&sb, "?url=%s/%s/%s", reponame, pagename,
			    (filename ? filename : ""));
		delim = "&amp;";
	}
	if (query)
		strbuf_addf(&sb, "%s%s", delim, query);
	return strbuf_detach(&sb, NULL);
}

char *cgit_pageurl(const char *reponame, const char *pagename,
		   const char *query)
{
	return cgit_fileurl(reponame, pagename, NULL, query);
}

const char *cgit_repobasename(const char *reponame)
{
	/* I assume we don't need to store more than one repo basename */
	static char rvbuf[1024];
	int p;
	const char *rv;
	strncpy(rvbuf, reponame, sizeof(rvbuf));
	if (rvbuf[sizeof(rvbuf)-1])
		die("cgit_repobasename: truncated repository name '%s'", reponame);
	p = strlen(rvbuf)-1;
	/* strip trailing slashes */
	while (p && rvbuf[p] == '/') rvbuf[p--] = 0;
	/* strip trailing .git */
	if (p >= 3 && starts_with(&rvbuf[p-3], ".git")) {
		p -= 3; rvbuf[p--] = 0;
	}
	/* strip more trailing slashes if any */
	while ( p && rvbuf[p] == '/') rvbuf[p--] = 0;
	/* find last slash in the remaining string */
	rv = strrchr(rvbuf,'/');
	if (rv)
		return ++rv;
	return rvbuf;
}

static void site_url(const char *page, const char *search, const char *sort, int ofs, int always_root)
{
	char *delim = "?";

	if (always_root || page)
		html_attr(cgit_rooturl());
	else
		html_attr(cgit_currenturl());

	if (page) {
		htmlf("?p=%s", page);
		delim = "&amp;";
	}
	if (search) {
		html(delim);
		html("q=");
		html_attr(search);
		delim = "&amp;";
	}
	if (sort) {
		html(delim);
		html("s=");
		html_attr(sort);
		delim = "&amp;";
	}
	if (ofs) {
		html(delim);
		htmlf("ofs=%d", ofs);
	}
}

static void site_link(const char *page, const char *name, const char *title,
		      const char *class, const char *search, const char *sort, int ofs, int always_root)
{
	html("<a");
	if (title) {
		html(" title='");
		html_attr(title);
		html("'");
	}
	if (class) {
		html(" class='");
		html_attr(class);
		html("'");
	}
	html(" href='");
	site_url(page, search, sort, ofs, always_root);
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_index_link(const char *name, const char *title, const char *class,
		     const char *pattern, const char *sort, int ofs, int always_root)
{
	site_link(NULL, name, title, class, pattern, sort, ofs, always_root);
}

static char *repolink(const char *title, const char *class, const char *page,
		      const char *head, const char *path)
{
	char *delim = "?";

	html("<a");
	if (title) {
		html(" title='");
		html_attr(title);
		html("'");
	}
	if (class) {
		html(" class='");
		html_attr(class);
		html("'");
	}
	html(" href='");
	if (ctx.cfg.virtual_root) {
		html_url_path(ctx.cfg.virtual_root);
		html_url_path(ctx.repo->url);
		if (ctx.repo->url[strlen(ctx.repo->url) - 1] != '/')
			html("/");
		if (page) {
			html_url_path(page);
			html("/");
			if (path)
				html_url_path(path);
		}
	} else {
		html_url_path(ctx.cfg.script_name);
		html("?url=");
		html_url_arg(ctx.repo->url);
		if (ctx.repo->url[strlen(ctx.repo->url) - 1] != '/')
			html("/");
		if (page) {
			html_url_arg(page);
			html("/");
			if (path)
				html_url_arg(path);
		}
		delim = "&amp;";
	}
	if (head && strcmp(head, ctx.repo->defbranch)) {
		html(delim);
		html("h=");
		html_url_arg(head);
		delim = "&amp;";
	}
	return fmt("%s", delim);
}

static void reporevlink(const char *page, const char *name, const char *title,
			const char *class, const char *head, const char *rev,
			const char *path)
{
	char *delim;

	delim = repolink(title, class, page, head, path);
	if (rev && ctx.qry.head != NULL && strcmp(rev, ctx.qry.head)) {
		html(delim);
		html("id=");
		html_url_arg(rev);
	}
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_summary_link(const char *name, const char *title, const char *class,
		       const char *head)
{
	reporevlink(NULL, name, title, class, head, NULL, NULL);
}

void cgit_tag_link(const char *name, const char *title, const char *class,
		   const char *tag)
{
	reporevlink("tag", name, title, class, tag, NULL, NULL);
}

void cgit_tree_link(const char *name, const char *title, const char *class,
		    const char *head, const char *rev, const char *path)
{
	reporevlink("tree", name, title, class, head, rev, path);
}

void cgit_plain_link(const char *name, const char *title, const char *class,
		     const char *head, const char *rev, const char *path)
{
	reporevlink("plain", name, title, class, head, rev, path);
}

void cgit_log_link(const char *name, const char *title, const char *class,
		   const char *head, const char *rev, const char *path,
		   int ofs, const char *grep, const char *pattern, int showmsg,
		   int follow)
{
	char *delim;

	delim = repolink(title, class, "log", head, path);
	if (rev && ctx.qry.head && strcmp(rev, ctx.qry.head)) {
		html(delim);
		html("id=");
		html_url_arg(rev);
		delim = "&amp;";
	}
	if (grep && pattern) {
		html(delim);
		html("qt=");
		html_url_arg(grep);
		delim = "&amp;";
		html(delim);
		html("q=");
		html_url_arg(pattern);
	}
	if (ofs > 0) {
		html(delim);
		html("ofs=");
		htmlf("%d", ofs);
		delim = "&amp;";
	}
	if (showmsg) {
		html(delim);
		html("showmsg=1");
		delim = "&amp;";
	}
	if (follow) {
		html(delim);
		html("follow=1");
	}
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_commit_link(char *name, const char *title, const char *class,
		      const char *head, const char *rev, const char *path)
{
	if (strlen(name) > ctx.cfg.max_msg_len && ctx.cfg.max_msg_len >= 15) {
		name[ctx.cfg.max_msg_len] = '\0';
		name[ctx.cfg.max_msg_len - 1] = '.';
		name[ctx.cfg.max_msg_len - 2] = '.';
		name[ctx.cfg.max_msg_len - 3] = '.';
	}

	char *delim;

	delim = repolink(title, class, "commit", head, path);
	if (rev && ctx.qry.head && strcmp(rev, ctx.qry.head)) {
		html(delim);
		html("id=");
		html_url_arg(rev);
		delim = "&amp;";
	}
	if (ctx.qry.difftype) {
		html(delim);
		htmlf("dt=%d", ctx.qry.difftype);
		delim = "&amp;";
	}
	if (ctx.qry.context > 0 && ctx.qry.context != 3) {
		html(delim);
		html("context=");
		htmlf("%d", ctx.qry.context);
		delim = "&amp;";
	}
	if (ctx.qry.ignorews) {
		html(delim);
		html("ignorews=1");
		delim = "&amp;";
	}
	if (ctx.qry.follow) {
		html(delim);
		html("follow=1");
	}
	html("'>");
	if (name[0] != '\0')
		html_txt(name);
	else
		html_txt("(no commit message)");
	html("</a>");
}

void cgit_refs_link(const char *name, const char *title, const char *class,
		    const char *head, const char *rev, const char *path)
{
	reporevlink("refs", name, title, class, head, rev, path);
}

void cgit_snapshot_link(const char *name, const char *title, const char *class,
			const char *head, const char *rev,
			const char *archivename)
{
	reporevlink("snapshot", name, title, class, head, rev, archivename);
}

void cgit_diff_link(const char *name, const char *title, const char *class,
		    const char *head, const char *new_rev, const char *old_rev,
		    const char *path)
{
	char *delim;

	delim = repolink(title, class, "diff", head, path);
	if (new_rev && ctx.qry.head != NULL && strcmp(new_rev, ctx.qry.head)) {
		html(delim);
		html("id=");
		html_url_arg(new_rev);
		delim = "&amp;";
	}
	if (old_rev) {
		html(delim);
		html("id2=");
		html_url_arg(old_rev);
		delim = "&amp;";
	}
	if (ctx.qry.difftype) {
		html(delim);
		htmlf("dt=%d", ctx.qry.difftype);
		delim = "&amp;";
	}
	if (ctx.qry.context > 0 && ctx.qry.context != 3) {
		html(delim);
		html("context=");
		htmlf("%d", ctx.qry.context);
		delim = "&amp;";
	}
	if (ctx.qry.ignorews) {
		html(delim);
		html("ignorews=1");
		delim = "&amp;";
	}
	if (ctx.qry.follow) {
		html(delim);
		html("follow=1");
	}
	html("'>");
	html_txt(name);
	html("</a>");
}

void cgit_patch_link(const char *name, const char *title, const char *class,
		     const char *head, const char *rev, const char *path)
{
	reporevlink("patch", name, title, class, head, rev, path);
}

void cgit_stats_link(const char *name, const char *title, const char *class,
		     const char *head, const char *path)
{
	reporevlink("stats", name, title, class, head, NULL, path);
}

static void cgit_self_link(char *name, const char *title, const char *class)
{
	if (!strcmp(ctx.qry.page, "repolist"))
		cgit_index_link(name, title, class, ctx.qry.search, ctx.qry.sort,
				ctx.qry.ofs, 1);
	else if (!strcmp(ctx.qry.page, "summary"))
		cgit_summary_link(name, title, class, ctx.qry.head);
	else if (!strcmp(ctx.qry.page, "tag"))
		cgit_tag_link(name, title, class, ctx.qry.has_sha1 ?
			       ctx.qry.sha1 : ctx.qry.head);
	else if (!strcmp(ctx.qry.page, "tree"))
		cgit_tree_link(name, title, class, ctx.qry.head,
			       ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
			       ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "plain"))
		cgit_plain_link(name, title, class, ctx.qry.head,
				ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
				ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "log"))
		cgit_log_link(name, title, class, ctx.qry.head,
			      ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
			      ctx.qry.path, ctx.qry.ofs,
			      ctx.qry.grep, ctx.qry.search,
			      ctx.qry.showmsg, ctx.qry.follow);
	else if (!strcmp(ctx.qry.page, "commit"))
		cgit_commit_link(name, title, class, ctx.qry.head,
				 ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
				 ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "patch"))
		cgit_patch_link(name, title, class, ctx.qry.head,
				ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
				ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "refs"))
		cgit_refs_link(name, title, class, ctx.qry.head,
			       ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
			       ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "snapshot"))
		cgit_snapshot_link(name, title, class, ctx.qry.head,
				   ctx.qry.has_sha1 ? ctx.qry.sha1 : NULL,
				   ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "diff"))
		cgit_diff_link(name, title, class, ctx.qry.head,
			       ctx.qry.sha1, ctx.qry.sha2,
			       ctx.qry.path);
	else if (!strcmp(ctx.qry.page, "stats"))
		cgit_stats_link(name, title, class, ctx.qry.head,
				ctx.qry.path);
	else {
		/* Don't known how to make link for this page */
		repolink(title, class, ctx.qry.page, ctx.qry.head, ctx.qry.path);
		html("><!-- cgit_self_link() doesn't know how to make link for page '");
		html_txt(ctx.qry.page);
		html("' -->");
		html_txt(name);
		html("</a>");
	}
}

void cgit_object_link(struct object *obj)
{
	char *page, *shortrev, *fullrev, *name;

	fullrev = sha1_to_hex(obj->sha1);
	shortrev = xstrdup(fullrev);
	shortrev[10] = '\0';
	if (obj->type == OBJ_COMMIT) {
		cgit_commit_link(fmt("commit %s...", shortrev), NULL, NULL,
				 ctx.qry.head, fullrev, NULL);
		return;
	} else if (obj->type == OBJ_TREE)
		page = "tree";
	else if (obj->type == OBJ_TAG)
		page = "tag";
	else
		page = "blob";
	name = fmt("%s %s...", typename(obj->type), shortrev);
	reporevlink(page, name, NULL, NULL, ctx.qry.head, fullrev, NULL);
}

static struct string_list_item *lookup_path(struct string_list *list,
					    const char *path)
{
	struct string_list_item *item;

	while (path && path[0]) {
		if ((item = string_list_lookup(list, path)))
			return item;
		if (!(path = strchr(path, '/')))
			break;
		path++;
	}
	return NULL;
}

void cgit_submodule_link(const char *class, char *path, const char *rev)
{
	struct string_list *list;
	struct string_list_item *item;
	char tail, *dir;
	size_t len;

	len = 0;
	tail = 0;
	list = &ctx.repo->submodules;
	item = lookup_path(list, path);
	if (!item) {
		len = strlen(path);
		tail = path[len - 1];
		if (tail == '/') {
			path[len - 1] = 0;
			item = lookup_path(list, path);
		}
	}
	if (item || ctx.repo->module_link) {
		html("<a ");
		if (class)
			htmlf("class='%s' ", class);
		html("href='");
		if (item) {
			html_attrf(item->util, rev);
		} else {
			dir = strrchr(path, '/');
			if (dir)
				dir++;
			else
				dir = path;
			html_attrf(ctx.repo->module_link, dir, rev);
		}
		html("'>");
		html_txt(path);
		html("</a>");
	} else {
		html("<span");
		if (class)
			htmlf(" class='%s'", class);
		html(">");
		html_txt(path);
		html("</span>");
	}
	html_txtf(" @ %.7s", rev);
	if (item && tail)
		path[len - 1] = tail;
}

static const char *fmt_date(time_t secs, const char *format, int local_time)
{
	static char buf[64];
	struct tm *time;

	if (!secs)
		return "";
	if (local_time)
		time = localtime(&secs);
	else
		time = gmtime(&secs);
	strftime(buf, sizeof(buf)-1, format, time);
	return buf;
}

void cgit_print_date(time_t secs, const char *format, int local_time)
{
	html_txt(fmt_date(secs, format, local_time));
}

static void print_rel_date(time_t t, double value,
	const char *class, const char *suffix)
{
	htmlf("<span class='%s' title='", class);
	html_attr(fmt_date(t, FMT_LONGDATE, ctx.cfg.local_time));
	htmlf("'>%.0f %s</span>", value, suffix);
}

void cgit_print_age(time_t t, time_t max_relative, const char *format)
{
	time_t now, secs;

	if (!t)
		return;
	time(&now);
	secs = now - t;
	if (secs < 0)
		secs = 0;

	if (secs > max_relative && max_relative >= 0) {
		html("<span title='");
		html_attr(fmt_date(t, FMT_LONGDATE, ctx.cfg.local_time));
		html("'>");
		cgit_print_date(t, format, ctx.cfg.local_time);
		html("</span>");
		return;
	}

	if (secs < TM_HOUR * 2) {
		print_rel_date(t, secs * 1.0 / TM_MIN, "age-mins", "min.");
		return;
	}
	if (secs < TM_DAY * 2) {
		print_rel_date(t, secs * 1.0 / TM_HOUR, "age-hours", "hours");
		return;
	}
	if (secs < TM_WEEK * 2) {
		print_rel_date(t, secs * 1.0 / TM_DAY, "age-days", "days");
		return;
	}
	if (secs < TM_MONTH * 2) {
		print_rel_date(t, secs * 1.0 / TM_WEEK, "age-weeks", "weeks");
		return;
	}
	if (secs < TM_YEAR * 2) {
		print_rel_date(t, secs * 1.0 / TM_MONTH, "age-months", "months");
		return;
	}
	print_rel_date(t, secs * 1.0 / TM_YEAR, "age-years", "years");
}

void cgit_print_http_headers(void)
{
	if (ctx.env.no_http && !strcmp(ctx.env.no_http, "1"))
		return;

	if (ctx.page.status)
		htmlf("Status: %d %s\n", ctx.page.status, ctx.page.statusmsg);
	if (ctx.page.mimetype && ctx.page.charset)
		htmlf("Content-Type: %s; charset=%s\n", ctx.page.mimetype,
		      ctx.page.charset);
	else if (ctx.page.mimetype)
		htmlf("Content-Type: %s\n", ctx.page.mimetype);
	if (ctx.page.size)
		htmlf("Content-Length: %zd\n", ctx.page.size);
	if (ctx.page.filename)
		htmlf("Content-Disposition: inline; filename=\"%s\"\n",
		      ctx.page.filename);
	if (!ctx.env.authenticated)
		html("Cache-Control: no-cache, no-store\n");
	htmlf("Last-Modified: %s\n", http_date(ctx.page.modified));
	htmlf("Expires: %s\n", http_date(ctx.page.expires));
	if (ctx.page.etag)
		htmlf("ETag: \"%s\"\n", ctx.page.etag);
	html("\n");
	if (ctx.env.request_method && !strcmp(ctx.env.request_method, "HEAD"))
		exit(0);
}

void cgit_redirect(const char *url, bool permanent)
{
	htmlf("Status: %d %s\n", permanent ? 301 : 302, permanent ? "Moved" : "Found");
	htmlf("Location: %s\n\n", url);
	exit(0);
}

static void print_rel_vcs_link(const char *url)
{
	html("<link rel='vcs-git' href='");
	html_attr(url);
	html("' title='");
	html_attr(ctx.repo->name);
	html(" Git repository'/>\n");
}

void cgit_print_docstart(void)
{
	if (ctx.cfg.embedded) {
		if (ctx.cfg.header)
			html_include(ctx.cfg.header);
		return;
	}

	const char *host = cgit_hosturl();
	html(cgit_doctype);
	html("<html xmlns='http://www.w3.org/1999/xhtml' xml:lang='en' lang='en'>\n");
	html("<head>\n");
	html("<title>");
	html_txt(ctx.page.title);
	html("</title>\n");
	htmlf("<meta name='generator' content='cgit %s'/>\n", cgit_version);
	if (ctx.cfg.robots && *ctx.cfg.robots)
		htmlf("<meta name='robots' content='%s'/>\n", ctx.cfg.robots);
	html("<link rel='stylesheet' type='text/css' href='");
	html_attr(ctx.cfg.css);
	html("'/>\n");
	if (ctx.cfg.favicon) {
		html("<link rel='shortcut icon' href='");
		html_attr(ctx.cfg.favicon);
		html("'/>\n");
	}
	if (host && ctx.repo && ctx.qry.head) {
		struct strbuf sb = STRBUF_INIT;
		strbuf_addf(&sb, "h=%s", ctx.qry.head);

		html("<link rel='alternate' title='Atom feed' href='");
		html(cgit_httpscheme());
		html_attr(cgit_hosturl());
		html_attr(cgit_fileurl(ctx.repo->url, "atom", ctx.qry.vpath,
				       sb.buf));
		html("' type='application/atom+xml'/>\n");
		strbuf_release(&sb);
	}
	if (ctx.repo)
		cgit_add_clone_urls(print_rel_vcs_link);
	if (ctx.cfg.head_include)
		html_include(ctx.cfg.head_include);
	html("</head>\n");
	html("<body>\n");
	if (ctx.cfg.header)
		html_include(ctx.cfg.header);
}

void cgit_print_docend(void)
{
	html("</div> <!-- class=content -->\n");
	if (ctx.cfg.embedded) {
		html("</div> <!-- id=cgit -->\n");
		if (ctx.cfg.footer)
			html_include(ctx.cfg.footer);
		return;
	}
	if (ctx.cfg.footer)
		html_include(ctx.cfg.footer);
	else {
		htmlf("<div class='footer'>generated by <a href='http://git.zx2c4.com/cgit/about/'>cgit %s</a> at ",
			cgit_version);
		cgit_print_date(time(NULL), FMT_LONGDATE, ctx.cfg.local_time);
		html("</div>\n");
	}
	html("</div> <!-- id=cgit -->\n");
	html("</body>\n</html>\n");
}

void cgit_print_error_page(int code, const char *msg, const char *fmt, ...)
{
	va_list ap;
	ctx.page.expires = ctx.cfg.cache_dynamic_ttl;
	ctx.page.status = code;
	ctx.page.statusmsg = msg;
	cgit_print_http_headers();
	cgit_print_docstart();
	cgit_print_pageheader();
	va_start(ap, fmt);
	cgit_vprint_error(fmt, ap);
	va_end(ap);
	cgit_print_docend();
}

void cgit_print_layout_start(void)
{
	cgit_print_http_headers();
	cgit_print_docstart();
	cgit_print_pageheader();
}

void cgit_print_layout_end(void)
{
	cgit_print_docend();
}

static void add_clone_urls(void (*fn)(const char *), char *txt, char *suffix)
{
	struct strbuf **url_list = strbuf_split_str(txt, ' ', 0);
	int i;

	for (i = 0; url_list[i]; i++) {
		strbuf_rtrim(url_list[i]);
		if (url_list[i]->len == 0)
			continue;
		if (suffix && *suffix)
			strbuf_addf(url_list[i], "/%s", suffix);
		fn(url_list[i]->buf);
	}

	strbuf_list_free(url_list);
}

void cgit_add_clone_urls(void (*fn)(const char *))
{
	if (ctx.repo->clone_url)
		add_clone_urls(fn, expand_macros(ctx.repo->clone_url), NULL);
	else if (ctx.cfg.clone_prefix)
		add_clone_urls(fn, ctx.cfg.clone_prefix, ctx.repo->url);
}

static int print_branch_option(const char *refname, const struct object_id *oid,
			       int flags, void *cb_data)
{
	char *name = (char *)refname;
	html_option(name, name, ctx.qry.head);
	return 0;
}

void cgit_add_hidden_formfields(int incl_head, int incl_search,
				const char *page)
{
	if (!ctx.cfg.virtual_root) {
		struct strbuf url = STRBUF_INIT;

		strbuf_addf(&url, "%s/%s", ctx.qry.repo, page);
		if (ctx.qry.vpath)
			strbuf_addf(&url, "/%s", ctx.qry.vpath);
		html_hidden("url", url.buf);
		strbuf_release(&url);
	}

	if (incl_head && ctx.qry.head && ctx.repo->defbranch &&
	    strcmp(ctx.qry.head, ctx.repo->defbranch))
		html_hidden("h", ctx.qry.head);

	if (ctx.qry.sha1)
		html_hidden("id", ctx.qry.sha1);
	if (ctx.qry.sha2)
		html_hidden("id2", ctx.qry.sha2);
	if (ctx.qry.showmsg)
		html_hidden("showmsg", "1");

	if (incl_search) {
		if (ctx.qry.grep)
			html_hidden("qt", ctx.qry.grep);
		if (ctx.qry.search)
			html_hidden("q", ctx.qry.search);
	}
}

static const char *hc(const char *page)
{
	return strcmp(ctx.qry.page, page) ? NULL : "active";
}

static void cgit_print_path_crumbs(char *path)
{
	char *old_path = ctx.qry.path;
	char *p = path, *q, *end = path + strlen(path);

	ctx.qry.path = NULL;
	cgit_self_link("root", NULL, NULL);
	ctx.qry.path = p = path;
	while (p < end) {
		if (!(q = strchr(p, '/')))
			q = end;
		*q = '\0';
		html_txt("/");
		cgit_self_link(p, NULL, NULL);
		if (q < end)
			*q = '/';
		p = q + 1;
	}
	ctx.qry.path = old_path;
}

static void print_header(void)
{
	char *logo = NULL, *logo_link = NULL;

	html("<table id='header'>\n");
	html("<tr>\n");

	if (ctx.repo && ctx.repo->logo && *ctx.repo->logo)
		logo = ctx.repo->logo;
	else
		logo = ctx.cfg.logo;
	if (ctx.repo && ctx.repo->logo_link && *ctx.repo->logo_link)
		logo_link = ctx.repo->logo_link;
	else
		logo_link = ctx.cfg.logo_link;
	if (logo && *logo) {
		html("<td class='logo' rowspan='2'><a href='");
		if (logo_link && *logo_link)
			html_attr(logo_link);
		else
			html_attr(cgit_rooturl());
		html("'><img src='");
		html_attr(logo);
		html("' alt='cgit logo'/></a></td>\n");
	}

	html("<td class='main'>");
	if (ctx.repo) {
		cgit_index_link("index", NULL, NULL, NULL, NULL, 0, 1);
		html(" : ");
		cgit_summary_link(ctx.repo->name, ctx.repo->name, NULL, NULL);
		if (ctx.env.authenticated) {
			html("</td><td class='form'>");
			html("<form method='get' action=''>\n");
			cgit_add_hidden_formfields(0, 1, ctx.qry.page);
			html("<select name='h' onchange='this.form.submit();'>\n");
			for_each_branch_ref(print_branch_option, ctx.qry.head);
			if (ctx.repo->enable_remote_branches)
				for_each_remote_ref(print_branch_option, ctx.qry.head);
			html("</select> ");
			html("<input type='submit' name='' value='switch'/>");
			html("</form>");
		}
	} else
		html_txt(ctx.cfg.root_title);
	html("</td></tr>\n");

	html("<tr><td class='sub'>");
	if (ctx.repo) {
		html_txt(ctx.repo->desc);
		html("</td><td class='sub right'>");
		html_txt(ctx.repo->owner);
	} else {
		if (ctx.cfg.root_desc)
			html_txt(ctx.cfg.root_desc);
		else if (ctx.cfg.index_info)
			html_include(ctx.cfg.index_info);
	}
	html("</td></tr></table>\n");
}

void cgit_print_pageheader(void)
{
	html("<div id='cgit'>");
	if (!ctx.env.authenticated || !ctx.cfg.noheader)
		print_header();

	html("<table class='tabs'><tr><td>\n");
	if (ctx.env.authenticated && ctx.repo) {
		if (ctx.repo->readme.nr)
			reporevlink("about", "about", NULL,
				    hc("about"), ctx.qry.head, NULL,
				    NULL);
		cgit_summary_link("summary", NULL, hc("summary"),
				  ctx.qry.head);
		cgit_refs_link("refs", NULL, hc("refs"), ctx.qry.head,
			       ctx.qry.sha1, NULL);
		cgit_log_link("log", NULL, hc("log"), ctx.qry.head,
			      NULL, ctx.qry.vpath, 0, NULL, NULL,
			      ctx.qry.showmsg, ctx.qry.follow);
		cgit_tree_link("tree", NULL, hc("tree"), ctx.qry.head,
			       ctx.qry.sha1, ctx.qry.vpath);
		cgit_commit_link("commit", NULL, hc("commit"),
				 ctx.qry.head, ctx.qry.sha1, ctx.qry.vpath);
		cgit_diff_link("diff", NULL, hc("diff"), ctx.qry.head,
			       ctx.qry.sha1, ctx.qry.sha2, ctx.qry.vpath);
		if (ctx.repo->max_stats)
			cgit_stats_link("stats", NULL, hc("stats"),
					ctx.qry.head, ctx.qry.vpath);
		html("</td><td class='form'>");
		html("<form class='right' method='get' action='");
		if (ctx.cfg.virtual_root)
			html_url_path(cgit_fileurl(ctx.qry.repo, "log",
						   ctx.qry.vpath, NULL));
		html("'>\n");
		cgit_add_hidden_formfields(1, 0, "log");
		html("<select name='qt'>\n");
		html_option("grep", "log msg", ctx.qry.grep);
		html_option("author", "author", ctx.qry.grep);
		html_option("committer", "committer", ctx.qry.grep);
		html_option("range", "range", ctx.qry.grep);
		html("</select>\n");
		html("<input class='txt' type='text' size='10' name='q' value='");
		html_attr(ctx.qry.search);
		html("'/>\n");
		html("<input type='submit' value='search'/>\n");
		html("</form>\n");
	} else if (ctx.env.authenticated) {
		site_link(NULL, "index", NULL, hc("repolist"), NULL, NULL, 0, 1);
		if (ctx.cfg.root_readme)
			site_link("about", "about", NULL, hc("about"),
				  NULL, NULL, 0, 1);
		html("</td><td class='form'>");
		html("<form method='get' action='");
		html_attr(cgit_currenturl());
		html("'>\n");
		html("<input type='text' name='q' size='10' value='");
		html_attr(ctx.qry.search);
		html("'/>\n");
		html("<input type='submit' value='search'/>\n");
		html("</form>");
	}
	html("</td></tr></table>\n");
	if (ctx.env.authenticated && ctx.qry.vpath) {
		html("<div class='path'>");
		html("path: ");
		cgit_print_path_crumbs(ctx.qry.vpath);
		if (ctx.cfg.enable_follow_links && !strcmp(ctx.qry.page, "log")) {
			html(" (");
			ctx.qry.follow = !ctx.qry.follow;
			cgit_self_link(ctx.qry.follow ? "follow" : "unfollow",
					NULL, NULL);
			ctx.qry.follow = !ctx.qry.follow;
			html(")");
		}
		html("</div>");
	}
	html("<div class='content'>");
}

void cgit_print_filemode(unsigned short mode)
{
	if (S_ISDIR(mode))
		html("d");
	else if (S_ISLNK(mode))
		html("l");
	else if (S_ISGITLINK(mode))
		html("m");
	else
		html("-");
	html_fileperm(mode >> 6);
	html_fileperm(mode >> 3);
	html_fileperm(mode);
}

void cgit_print_snapshot_links(const char *repo, const char *head,
			       const char *hex, int snapshots)
{
	const struct cgit_snapshot_format* f;
	struct strbuf filename = STRBUF_INIT;
	size_t prefixlen;
	unsigned char sha1[20];

	if (get_sha1(fmt("refs/tags/%s", hex), sha1) == 0 &&
	    (hex[0] == 'v' || hex[0] == 'V') && isdigit(hex[1]))
		hex++;
	strbuf_addf(&filename, "%s-%s", cgit_repobasename(repo), hex);
	prefixlen = filename.len;
	for (f = cgit_snapshot_formats; f->suffix; f++) {
		if (!(snapshots & f->bit))
			continue;
		strbuf_setlen(&filename, prefixlen);
		strbuf_addstr(&filename, f->suffix);
		cgit_snapshot_link(filename.buf, NULL, NULL, NULL, NULL,
				   filename.buf);
		html("<br/>");
	}
	strbuf_release(&filename);
}

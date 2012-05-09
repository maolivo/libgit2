#include "filter.h"
	if (git_attr_get(repo, 0, file->path, "diff", &value) < 0)
	git_buf search;
	git_text_stats stats;
		search.ptr  = old_data->data;
		search.size = min(old_data->len, 4000);

		git_text_gather_stats(&stats, &search);

		if (git_text_is_binary(&stats))
		search.ptr  = new_data->data;
		search.size = min(new_data->len, 4000);

		git_text_gather_stats(&stats, &search);

		if (git_text_is_binary(&stats))
static void fill_map_from_mmfile(git_map *dst, mmfile_t *src) {
	assert(dst && src);

	dst->data = src->ptr;
	dst->len = src->size;
#ifdef GIT_WIN32
	dst->fmh = NULL;
#endif
}

		mmfile_t old_xdiff_data, new_xdiff_data;



				delta, &old_data, &new_data);
		old_xdiff_data.ptr = old_data.data;
		old_xdiff_data.size = old_data.len;
		new_xdiff_data.ptr = new_data.data;
		new_xdiff_data.size = new_data.len;
		xdl_diff(&old_xdiff_data, &new_xdiff_data,
	git_oid_tostr(start_oid, sizeof(start_oid), &delta->old_file.oid);
	git_oid_tostr(end_oid, sizeof(end_oid), &delta->new_file.oid);
	git_diff_file_fn file_cb,
	git_map old_map, new_map;
	git_blob *new, *old;
	memset(&delta, 0, sizeof(delta));

	new = new_blob;
	old = old_blob;
		git_blob *swap = old;
		old = new;
		new = swap;
	if (old) {
		old_data.ptr  = (char *)git_blob_rawcontent(old);
		old_data.size = git_blob_rawsize(old);
		git_oid_cpy(&delta.old_file.oid, git_object_id((const git_object *)old));
	if (new) {
		new_data.ptr  = (char *)git_blob_rawcontent(new);
		new_data.size = git_blob_rawsize(new);
		git_oid_cpy(&delta.new_file.oid, git_object_id((const git_object *)new));
	delta.status = new ?
		(old ? GIT_DELTA_MODIFIED : GIT_DELTA_ADDED) :
		(old ? GIT_DELTA_DELETED : GIT_DELTA_UNTRACKED);

	if (git_oid_cmp(&delta.new_file.oid, &delta.old_file.oid) == 0)
		delta.status = GIT_DELTA_UNMODIFIED;

	delta.old_file.size = old_data.size;
	delta.new_file.size = new_data.size;

	fill_map_from_mmfile(&old_map, &old_data);
	fill_map_from_mmfile(&new_map, &new_data);

	if (file_is_binary_by_content(&delta, &old_map, &new_map) < 0)
		return -1;

	if (file_cb != NULL) {
		int error = file_cb(cb_data, &delta, 1);
		if (error < 0)
			return error;
	}

	/* don't do hunk and line diffs if the two blobs are identical */
	if (delta.status == GIT_DELTA_UNMODIFIED)
		return 0;

	/* don't do hunk and line diffs if file is binary */
	if (delta.binary == 1)
		return 0;
/*
* Copyright (C) the libgit2 contributors. All rights reserved.
*
* This file is part of libgit2, distributed under the GNU GPL v2 with
* a Linking Exception. For full terms see the included COPYING file.
*/

#include "clar_libgit2.h"

#include "common.h"
#include "path.h"
#include "hooks.h"
#include "repository.h"

#define REPO_PATH "hookstestrepo"

static git_repository *_repo = NULL;
static int _expected_hooks[GIT_HOOK_TYPE_MAXIMUM_SUPPORTED];

void test_hooks_get__initialize(void)
{
    int i;

    cl_assert_equal_i(ARRAY_SIZE(supported_hooks), GIT_HOOK_TYPE_MAXIMUM_SUPPORTED);
    cl_assert_equal_i(ARRAY_SIZE(supported_hooks), ARRAY_SIZE(_expected_hooks));

    for (i = 0; i < ARRAY_SIZE(_expected_hooks); i++) {
        _expected_hooks[i] = GIT_HOOK_FALSE;
    }

    cl_assert(!git_path_isdir(REPO_PATH));

    cl_git_pass(git_repository_init(&_repo, REPO_PATH, 0));

    cl_assert(git_repository_is_empty(_repo));
}

void test_hooks_get__cleanup(void)
{
    git_repository_free(_repo);
    _repo = NULL;

    cl_fixture_cleanup(REPO_PATH);
}

static git_buf get_hook_path()
{
    git_buf hook_path;

    git_buf_init(&hook_path, 0);
    cl_assert(git_buf_joinpath(&hook_path, _repo->path_repository, GIT_HOOKS_DIRECTORY_NAME) == 0);
    return hook_path;
}

static void verify_path_for_hook(git_repository_hook *hook, git_hook_type type)
{
    git_buf hook_path = get_hook_path();
    const char *file_name = supported_hooks[type];

    cl_assert(git_buf_joinpath(&hook_path, git_buf_cstr(&hook_path), file_name) == 0);

    cl_assert_equal_s(git_buf_cstr(&hook->path), git_buf_cstr(&hook_path));

    git_buf_free(&hook_path);
}

static git_repository_hook* get_hook(git_hook_type type)
{
    git_repository_hook *hook;

    cl_git_pass(git_repository_hook_get(&hook, _repo, type));

    cl_assert_equal_i(hook->type, type);

    verify_path_for_hook(hook, type);

    return hook;
}

static void get_hooks_and_check_and_cleanup()
{
    int i;

    for (i = 0; i < GIT_HOOK_TYPE_MAXIMUM_SUPPORTED; i++) {
        git_repository_hook *hook = get_hook(i);

        cl_assert_equal_i(hook->exists, _expected_hooks[i]);

        git_repository_hook_free(hook);
    }
}

static void add_hook_file(git_hook_type type)
{
    const char *file_name = supported_hooks[type];
    git_buf hook_file_path = get_hook_path();

    cl_assert(git_buf_joinpath(&hook_file_path, git_buf_cstr(&hook_file_path), file_name) == 0);

    cl_git_mkfile(git_buf_cstr(&hook_file_path), "Test");

    git_buf_free(&hook_file_path);
}

void test_hooks_get__verify_no_hooks(void)
{
    get_hooks_and_check_and_cleanup();
}

void test_hooks_get__verify_some_hooks(void)
{
    add_hook_file(GIT_HOOK_TYPE_COMMIT_MSG);
    _expected_hooks[GIT_HOOK_TYPE_COMMIT_MSG] = GIT_HOOK_TRUE;

    add_hook_file(GIT_HOOK_TYPE_PRE_PUSH);
    _expected_hooks[GIT_HOOK_TYPE_PRE_PUSH] = GIT_HOOK_TRUE;

    add_hook_file(GIT_HOOK_TYPE_POST_UPDATE);
    _expected_hooks[GIT_HOOK_TYPE_POST_UPDATE] = GIT_HOOK_TRUE;

    get_hooks_and_check_and_cleanup();
}

void test_hooks_get__verify_all_hooks(void)
{
    int i;

    /**
    * Explicitly assume all hooks should be present. This helps to validate
    * that not only all the hooks are present, but that we are not missing
    * any hooks that need to be added in this test.
    */
    for (i = 0; i < ARRAY_SIZE(_expected_hooks); i++) {
        add_hook_file(i);
        _expected_hooks[i] = GIT_HOOK_TRUE;
    }

    get_hooks_and_check_and_cleanup();
}

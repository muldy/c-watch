def options(ctx):
    ctx.add_option('--no-logs',action='store_true',default=False,
            help="Mark a build to exclude app logging output")

def configure(ctx):
    if ctx.options.no_logs:
        ctx.env.append_value('DEFINES','NOLOG')

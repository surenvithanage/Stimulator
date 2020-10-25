import os
from argparse import ArgumentParser
from sklearn.datasets import make_blobs

if __name__ == '__main__':

	# Parse args
	parser = ArgumentParser()
	parser.add_argument(
		'-n', metavar='N', type=int, required=True,
		help='Number of values to generate.',
	)
	parser.add_argument(
		'-k', metavar='K', type=int, required=False,
		help='Number of centroids.',
	)
	parser.add_argument(
		'-o', metavar='OUTPUT', required=True,
		help='Directory to which the ouptut must be produced.',
	)
	parser.add_argument(
		'-g', required=False, action='store_true',
		help='Outputs a .gitignore file that excludes everything in the output directory.',
	)
	args = parser.parse_args()

	# Make blobs
	x, y, centroids = make_blobs(
		n_samples=args.n, centers=args.k,
		n_features=1, return_centers=True, center_box=(0, 100),
	)

	# Convert [[a], [b]] to [a, b] (n_features=1)
	x = [x for [x] in x]
	centroids = [c for [c] in centroids]

	# Output
	def write(path, contents):
		print(f'Writing {path}')
		with open(path, 'w') as f:
			f.writelines(contents)

	os.makedirs(args.o, exist_ok=True)
	write(f'{args.o}/values', [str(x) + '\n' for x in x])
	write(f'{args.o}/memberships_o', [str(y) + '\n' for y in y])
	write(f'{args.o}/centroids_o', [str(c) + '\n' for c in centroids])
	if args.g: write(f'{args.o}/.gitignore', ['*\n', ''])
